import pandas as pd
import numpy as np

def recourse_choice_df(dataset_with_cfs):
    df = dataset_with_cfs.copy().drop(columns=['cfs_list'])
    df['recourse_choice'] = dataset_with_cfs['cfs_list'].apply(len).astype(int)
    return df

def recourse_availability_df(dataset_with_cfs):
    df = dataset_with_cfs.copy().drop(columns=['cfs_list'])
    df['recourse_availability'] = dataset_with_cfs['cfs_list'].apply(lambda x: 1 if x.size > 0 else 0).astype(int)
    return df

def cf_cost(counterfactual_features, sample_features, feature_types, dataset, features_to_perturb, feature_details_dict, label_sentiment_binary): # add feature_weights?
    cost = 0
    # cost depends on "sentiment" of a feature change, not only on a sentiment of the label to flip (whether we have a recourse to desirable or undesirable outcome)

    # if label_sentiment_binary == 1:
    #   for j in range(len(sample_features)):
    #     if feature_types[j] == "ordinal":
    #       if counterfactual_features[j] > sample_features[j]:
    #         min = feature_details_dict[features_to_perturb[j]]['range_min']
    #         max = feature_details_dict[features_to_perturb[j]]['range_max']
    #         cost += abs((counterfactual_features[j] - sample_features[j])/(max - min))
    #     elif feature_types[j] == "categorical":
    #       if counterfactual_features[j] != sample_features[j]:
    #         cost += 1
    #     elif feature_types[j] == "numerical":
    #       if counterfactual_features[j] > sample_features[j]:
    #         min = feature_details_dict[features_to_perturb[j]]['range_min']
    #         min = feature_details_dict[features_to_perturb[j]]['range_max']
    #         cost += abs((counterfactual_features[j] - min)/(max - min) - (sample_features[j] - min)/(max - min))
    # if label_sentiment_binary == 0:
    #   for j in range(len(sample_features)):
    #     if feature_types[j] == "ordinal":
    #       if counterfactual_features[j] < sample_features[j]:
    #         min = feature_details_dict[features_to_perturb[j]]['range_min']
    #         max = feature_details_dict[features_to_perturb[j]]['range_max']
    #         cost += abs((counterfactual_features[j] - sample_features[j])/(max - min))
    #     elif feature_types[j] == "categorical":
    #       if counterfactual_features[j] != sample_features[j]:
    #         cost += 1
    #     elif feature_types[j] == "numerical":
    #       if counterfactual_features[j] < sample_features[j]:
    #         min = feature_details_dict[features_to_perturb[j]]['range_min']
    #         min = feature_details_dict[features_to_perturb[j]]['range_max']
    #         cost += abs((counterfactual_features[j] - min)/(max - min) - (sample_features[j] - min)/(max - min))
       
    for j in range(len(sample_features)):
        if feature_types[j] == "ordinal":
          # to add in future: define the max and min beforehand;
            min = feature_details_dict[features_to_perturb[j]]['range_min']
            max = feature_details_dict[features_to_perturb[j]]['range_max']
            cost += abs((counterfactual_features[j] - sample_features[j])/(max - min))
        elif feature_types[j] == "categorical":
          if counterfactual_features[j] != sample_features[j]:
            cost += 1
        elif feature_types[j] == "numerical":
            min = feature_details_dict[features_to_perturb[j]]['range_min']
            min = feature_details_dict[features_to_perturb[j]]['range_max']
            cost += abs((counterfactual_features[j] - min)/(max - min) - (sample_features[j] - min)/(max - min))
    # return cost/len(sample_features) - final normalization to 0-1
    return cost

def recourse_cost_df(dataset_with_cfs, features_dict, label_sentiment_binary):
    feature_names = list(features_dict.keys())
    feature_indices = [dataset_with_cfs.columns.get_loc(col) for col in feature_names]
    feature_types = [features_dict[feature]['type'] for feature in feature_names]

    inf_cost = 100
    a = np.ones(dataset_with_cfs.shape[0])*inf_cost
    for i in range(dataset_with_cfs.shape[0]):

      if dataset_with_cfs['cfs_list'].iloc[i].size == 0:
        continue
      sample_features = dataset_with_cfs.iloc[i, :-1].to_numpy()[feature_indices]
      cfs = np.array(dataset_with_cfs['cfs_list'].iloc[i])[:, feature_indices]

      min_cost = inf_cost
      for cf_features in cfs:
        # to add: if counterfactual is within a feature range
        # print(cf_features)
        cost = cf_cost(cf_features, sample_features, feature_types, dataset_with_cfs, feature_names, features_dict, label_sentiment_binary)
        min_cost = min(min_cost, cost)
      a[i] = min_cost

    # print(a)
    df = dataset_with_cfs.copy().drop(columns=['cfs_list'])
    df['recourse_cost'] = np.round(a, 2)
    return df[df.recourse_cost != inf_cost]