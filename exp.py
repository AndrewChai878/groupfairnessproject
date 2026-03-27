import subprocess
import pandas as pd
import os, time, shutil, datetime

BOOST_INCLUDE_CANDIDATES = [
    os.environ.get("BOOST_INCLUDE"),
    "/opt/homebrew/opt/boost/include",
    "/usr/local/opt/boost/include",
    r"C:/ProgramData/chocolatey/lib/boost-msvc-14.3/include/boost-1_84",
]


def _boost_include_args():
    for include_dir in BOOST_INCLUDE_CANDIDATES:
        if include_dir and os.path.isdir(include_dir):
            return [f"-I{include_dir}"]
    return []

def calculate_table(output_name, data_file_path, exp_patterns_path, num_patterns=15, min_support_param=0.01):
    # print('Explanation Tables pattern mining ...')

    # if os.path.exists(exp_patterns_path):
    #     shutil.rmtree(exp_patterns_path)
    # os.makedirs(exp_patterns_path)

    df = pd.read_csv(data_file_path)
    concepts_meta_cols = ['targetcol', 'pred', 'label', 'recourse', 'recourse_choice', 'recourse_support',
                          'recourse_cost', 'recourse_availability', 'unlearning_count']

    concept_cols = list(set(df.columns) - set(concepts_meta_cols))
    num_concepts = len(concept_cols)    
    remove_inactivated_patterns_num = 0

    current_path = os.path.abspath(os.path.dirname(__file__))
    # explanations_path = os.path.join(current_path, 'exp', 'backup', 'Explanations.cpp') ## to use LENS on binary data
    # lighthouse_path = os.path.join(current_path, 'exp', 'backup', 'Lighthouse.cpp')
    explanations_path = os.path.join(current_path, 'exp', 'Explanations.cpp')
    lighthouse_path = os.path.join(current_path, 'exp', 'Lighthouse.cpp')
    commands = ["g++", explanations_path, lighthouse_path, "-std=c++17", "-o", "program", *_boost_include_args()]
    print("Compiling with commands: ", commands)

    res = subprocess.run(commands, capture_output=True, universal_newlines=True)
    if res.stderr:
        print('Compilation return code:', res.returncode)
        print('Compilation output:', res.stdout)
        print('Compilation error:', res.stderr)

    t_start = datetime.datetime.now()
    #output_path = os.path.join(exp_patterns_path, output_name + str(min_support_param) + '.csv')
    output_path = exp_patterns_path
    print('Arguments to the program: {} {} {} {} {} {} {}'.format(output_name, data_file_path, 
            num_concepts, num_patterns, remove_inactivated_patterns_num, output_path, min_support_param))

    time.sleep(5)
    res = subprocess.run(["./program", output_name, data_file_path, str(num_concepts), str(num_patterns), 
        str(remove_inactivated_patterns_num), output_path, str(min_support_param)], capture_output=True, universal_newlines=True)
    if res.stderr:
        print('Execution return code:', res.returncode)
        print('Execution error:', res.stderr)

    # print('Execution output:', res.stdout)
    t_end = datetime.datetime.now()
    print('Time:', t_end - t_start)

    return output_path


if __name__ == "__main__":
    ...