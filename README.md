# EECS 6320 (Fairness & Bias in AI) - Group Project

This repo represents the work of our group project for EECS 6320, in which we addressed the issue of strip searches during arrests (see the presentation linked below for more details).

_Group_: G2 (Andrew Chai, Michael Murphy, Om Patel, Tirth Patel)

_Title_: Analysing and reducing recourse bias in strip searches

_Presentation_: [https://drive.google.com/file/d/1ZowOEQo-qJiDwY_1efXHdkFIRgutBBE-/view?usp=sharing](https://drive.google.com/file/d/1ZowOEQo-qJiDwY_1efXHdkFIRgutBBE-/view?usp=sharing)

## Prerequisites
- Python v3.12 (required for the fixed versions of the Python packages referenced in `requirements.txt`).
- [Boost v1.84.x](https://www.boost.org/releases/1.84.0/) (make note of the path where Boost is installed, or where you store it).

## Setup
1. Clone this repository.
2. Install required Python packages (`pip install -r requirements.txt`).
3. Add the path where Boost is installed/stored to the `BOOST_INCLUDE_CANDIDATES` list in `exp.py` (see [here](https://github.com/AndrewChai878/groupfairnessproject/blob/da75ab1872ba494c2d8e605598354b8c9a7f5b21/exp.py#L5)).

## Usage
1. Run all cells of `group_proj_classifier_nn_baseline.ipynb` from start to finish. The notebook will produce the results and visuals shown in the presentation.
