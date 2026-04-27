import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/josem/SeaSTAR/src/my_package_name/install/my_package_name'
