import copy

import numpy as np
import torch
import torch.nn as nn
from imblearn.over_sampling import SMOTE
from sklearn.metrics import accuracy_score, f1_score, roc_auc_score
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from torch.autograd import Function
from torch.utils.data import DataLoader, TensorDataset


class _GradientReversal(Function):
    @staticmethod
    def forward(ctx, x, lambda_):
        ctx.lambda_ = lambda_
        return x.view_as(x)

    @staticmethod
    def backward(ctx, grad_output):
        return -ctx.lambda_ * grad_output, None


def gradient_reverse(x, lambda_=1.0):
    return _GradientReversal.apply(x, lambda_)


def _dense_array(x):
    if hasattr(x, "toarray"):
        return x.toarray()
    return np.asarray(x)


def prepare_smote_case_with_sensitive_labels(
    case_name,
    preprocessor,
    x_train_raw,
    x_test_raw,
    y_train,
    y_test,
    protected_cols,
    random_state=42,
):
    xtr_enc = _dense_array(preprocessor.fit_transform(x_train_raw))
    xte_enc = _dense_array(preprocessor.transform(x_test_raw))

    smote = SMOTE(random_state=random_state)
    xtr_smote, ytr_smote = smote.fit_resample(xtr_enc, np.asarray(y_train))

    feature_names = list(preprocessor.get_feature_names_out())
    protected_targets = {}
    protected_classes = {}

    for col in protected_cols:
        prefix = f"cat_ohe__{col}_"
        indices = [i for i, name in enumerate(feature_names) if name.startswith(prefix)]
        if not indices:
            raise ValueError(f"Could not locate one-hot encoded columns for protected attribute '{col}'.")

        block = xtr_smote[:, indices]
        if len(indices) == 1:
            protected_targets[col] = (block[:, 0] >= 0.5).astype(np.int64)
            protected_classes[col] = [feature_names[indices[0]].split(prefix, 1)[-1]]
        else:
            protected_targets[col] = np.argmax(block, axis=1).astype(np.int64)
            protected_classes[col] = [feature_names[i].split(prefix, 1)[-1] for i in indices]

    scaler = StandardScaler()
    xtr_np = scaler.fit_transform(xtr_smote).astype(np.float32)
    xte_np = scaler.transform(xte_enc).astype(np.float32)

    return {
        "name": case_name,
        "preprocessor": preprocessor,
        "scaler": scaler,
        "X_train_np": xtr_np,
        "X_test_np": xte_np,
        "y_train_np": np.asarray(ytr_smote, dtype=np.float32),
        "y_test_np": np.asarray(y_test, dtype=np.float32),
        "protected_targets": protected_targets,
        "protected_classes": protected_classes,
        "protected_cols": list(protected_cols),
    }


def make_adversarial_loaders(
    x_train_np,
    y_train_np,
    protected_targets,
    batch_size=512,
    seed=42,
    val_size=0.1,
):
    indices = np.arange(len(y_train_np))
    train_idx, val_idx = train_test_split(
        indices,
        test_size=val_size,
        random_state=seed,
        stratify=np.asarray(y_train_np).astype(int),
    )

    protected_order = list(protected_targets.keys())

    train_tensors = [
        torch.tensor(x_train_np[train_idx], dtype=torch.float32),
        torch.tensor(np.asarray(y_train_np)[train_idx], dtype=torch.float32),
    ]
    val_tensors = [
        torch.tensor(x_train_np[val_idx], dtype=torch.float32),
        torch.tensor(np.asarray(y_train_np)[val_idx], dtype=torch.float32),
    ]

    for name in protected_order:
        arr = np.asarray(protected_targets[name], dtype=np.int64)
        train_tensors.append(torch.tensor(arr[train_idx], dtype=torch.long))
        val_tensors.append(torch.tensor(arr[val_idx], dtype=torch.long))

    train_ds = TensorDataset(*train_tensors)
    val_ds = TensorDataset(*val_tensors)

    return (
        DataLoader(train_ds, batch_size=batch_size, shuffle=True),
        DataLoader(val_ds, batch_size=batch_size, shuffle=False),
        protected_order,
    )


class MultiAdversarialMLP(nn.Module):
    def __init__(self, input_dim, protected_output_dims):
        super().__init__()
        self.feature_extractor = nn.Sequential(
            nn.Linear(input_dim, 128),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Dropout(0.2),
        )
        self.label_head = nn.Linear(64, 1)
        self.adversaries = nn.ModuleDict(
            {
                name: nn.Sequential(
                    nn.Linear(64, 32),
                    nn.ReLU(),
                    nn.Linear(32, out_dim),
                )
                for name, out_dim in protected_output_dims.items()
            }
        )

    def forward(self, x, adv_lambda=1.0):
        features = self.feature_extractor(x)
        label_logits = self.label_head(features).squeeze(1)
        reversed_features = gradient_reverse(features, adv_lambda)
        adv_logits = {name: head(reversed_features) for name, head in self.adversaries.items()}
        return label_logits, adv_logits


def train_multi_adversarial_model(
    model,
    train_loader,
    val_loader,
    protected_order,
    device,
    epochs=40,
    lr=1e-3,
    weight_decay=1e-4,
    patience=6,
    adv_lambda=1.0,
    adv_weight=1.0,
):
    task_criterion = nn.BCEWithLogitsLoss()
    adv_criterion = nn.CrossEntropyLoss()
    optimizer = torch.optim.Adam(model.parameters(), lr=lr, weight_decay=weight_decay)

    best_state = copy.deepcopy(model.state_dict())
    best_val_loss = float("inf")
    patience_counter = 0
    history = []

    for epoch in range(epochs):
        ramp = adv_lambda * float(epoch + 1) / float(max(epochs, 1))

        model.train()
        train_task_losses = []
        train_adv_losses = []

        for batch in train_loader:
            x_batch = batch[0].to(device)
            y_batch = batch[1].to(device)
            protected_batches = {
                protected_order[i]: batch[i + 2].to(device) for i in range(len(protected_order))
            }

            optimizer.zero_grad()
            label_logits, adv_logits = model(x_batch, adv_lambda=ramp)

            task_loss = task_criterion(label_logits, y_batch)
            adv_loss = sum(
                adv_criterion(adv_logits[name], protected_batches[name]) for name in protected_order
            )
            loss = task_loss + adv_weight * adv_loss
            loss.backward()
            optimizer.step()

            train_task_losses.append(task_loss.item())
            train_adv_losses.append(adv_loss.item())

        model.eval()
        val_task_losses = []
        val_adv_losses = []
        with torch.no_grad():
            for batch in val_loader:
                x_batch = batch[0].to(device)
                y_batch = batch[1].to(device)
                protected_batches = {
                    protected_order[i]: batch[i + 2].to(device) for i in range(len(protected_order))
                }

                label_logits, adv_logits = model(x_batch, adv_lambda=ramp)
                val_task_loss = task_criterion(label_logits, y_batch)
                val_adv_loss = sum(
                    adv_criterion(adv_logits[name], protected_batches[name]) for name in protected_order
                )
                val_task_losses.append(val_task_loss.item())
                val_adv_losses.append(val_adv_loss.item())

        avg_train_task = float(np.mean(train_task_losses))
        avg_train_adv = float(np.mean(train_adv_losses))
        avg_val_task = float(np.mean(val_task_losses))
        avg_val_adv = float(np.mean(val_adv_losses))

        history.append(
            {
                "epoch": epoch + 1,
                "adv_lambda": ramp,
                "train_task_loss": avg_train_task,
                "train_adv_loss": avg_train_adv,
                "val_task_loss": avg_val_task,
                "val_adv_loss": avg_val_adv,
            }
        )

        print(
            f"Epoch {epoch + 1}/{epochs} | "
            f"task_train={avg_train_task:.4f} | adv_train={avg_train_adv:.4f} | "
            f"task_val={avg_val_task:.4f} | adv_val={avg_val_adv:.4f} | "
            f"lambda={ramp:.3f}"
        )

        if avg_val_task < best_val_loss:
            best_val_loss = avg_val_task
            best_state = copy.deepcopy(model.state_dict())
            patience_counter = 0
        else:
            patience_counter += 1
            if patience_counter >= patience:
                print("Early stopping triggered.")
                break

    model.load_state_dict(best_state)
    return history


def evaluate_multi_adversarial_model(model, x_test_np, y_test_np, device, adv_lambda=1.0):
    model.eval()
    with torch.no_grad():
        xb = torch.tensor(x_test_np, dtype=torch.float32, device=device)
        label_logits, _ = model(xb, adv_lambda=adv_lambda)
        probs = torch.sigmoid(label_logits).cpu().numpy()

    y_pred = (probs >= 0.5).astype(int)
    y_true = np.asarray(y_test_np).astype(int).reshape(-1)

    return {
        "y_pred": y_pred,
        "y_prob": probs,
        "accuracy": accuracy_score(y_true, y_pred),
        "f1": f1_score(y_true, y_pred),
        "roc_auc": roc_auc_score(y_true, probs),
    }
