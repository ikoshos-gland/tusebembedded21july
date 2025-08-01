
import os
import sys
import argparse

SCRIPT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.append(os.path.dirname(SCRIPT_DIR))
from common.training import plot_learning_rate_schedule


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config-path', type=str, default='', help='Path to folder containing the configuration file')
    parser.add_argument('--config-name', type=str, default='user_config.yaml', help='Name of the configuration file')
    parser.add_argument("--fname", type=str, default="",  help="Path to output plot file (.png or any other format supported by matplotlib savefig())")
    args = parser.parse_args()
    
    #config_file_path = os.path.join(os.pardir, args.config_file)
    config_file_path = os.path.join(args.config_path, args.config_name)
    if not os.path.isfile(config_file_path):
        raise ValueError(f"\nUnable to find the YAML configuration file\nReceived path: {args.config_name}")
    fname = args.fname if args.fname else None

    # Call plot_learning_rate_schedule common routine 
    plot_learning_rate_schedule(config_file_path = config_file_path,
                                fname = fname)


if __name__ == '__main__':
    main()
