from setuptools import find_packages, setup

package_name = 'my_package_name'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='josem',
    maintainer_email='josem@todo.todo',
    description='TODO: Package description',
    license='TODO: License declaration',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        "console_scripts": [
            "mission_fsm_node = my_package_name.mission_fsm_node:main",
            "uart_bridge_node = my_package_name.uart_bridge_node:main",
            "mission_bridge_node = my_package_name.mission_bridge_node:main",
            "controltest = my_package_name.controltest:main",
            "camerafake = my_package_name.camerafake:main",
            "seastar_heartbeat_node = my_package_name.seastar_heartbeat_node:main",
            "gamecontroller = my_package_name.gamecontroller:main",
        ],
    },
)
