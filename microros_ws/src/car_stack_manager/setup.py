from setuptools import find_packages, setup

package_name = 'car_stack_manager'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ROS 2 Car Maintainers',
    maintainer_email='maintainer@example.com',
    description='Vehicle-side process manager exposed through ROS 2 DDS.',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'car_stack_manager = car_stack_manager.manager:main',
        ],
    },
)
