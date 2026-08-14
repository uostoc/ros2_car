from setuptools import find_packages, setup

package_name = 'car_platform_bringup'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', ['launch/vehicle_stack.launch.py']),
        ('share/' + package_name + '/config', ['config/vehicle.example.yaml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ROS 2 Car Maintainers',
    maintainer_email='maintainer@example.com',
    description='Unified launch entry points for the physical ROS 2 car.',
    license='Apache-2.0',
)
