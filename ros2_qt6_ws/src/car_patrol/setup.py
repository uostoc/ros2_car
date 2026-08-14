from setuptools import find_packages, setup

package_name = 'car_patrol'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/config', ['config/patrol_routes.yaml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ROS 2 Car Maintainers',
    maintainer_email='maintainer@example.com',
    description='Navigation-only route patrol server for the ROS 2 car.',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'car_patrol = car_patrol.patrol_server:main',
        ],
    },
)
