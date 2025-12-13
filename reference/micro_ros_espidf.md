# 为了使用esp-rs实现语音识别，实现指哪里打哪里的功能
需要一个 IIC 的麦克风模块，如INMP441 麦克风</br>
我们要使用 espidf 重构我们的代码
```bash
    # 安装到 ~/esp 目录
    mkdir -p ~/esp
    cd ~/esp
    # 克隆 ESP-IDF（推荐 v5.2，稳定且支持 micro-ROS）
    git clone -b v5.2.2 --recursive https://github.com/espressif/esp-idf.git
    # 安装工具链
    cd esp-idf
    ./install.sh esp32s3

    # 设置环境变量（每次打开终端都要执行，或加到 .bashrc）
    source ./export.sh
```
创建项目目录
```bash
    # 创建项目目录
    mkdir -p ~/esp/servo_voice_control/main
    cd ~/esp/servo_voice_control

# 克隆 micro-ROS 组件
    mkdir -p components
    cd components
    git clone https://github.com/micro-ROS/micro_ros_espidf_component.git
    cd ..
```
# 项目结构如下图所示
```bash
@heisd ➜ /workspaces/LabRecord/esp/servo_voice_control (liquanyan) $ tree
.
├── CmakeLists.txt
├── components
│   └── micro_ros_espidf_component
│       ├── 3rd-party-licenses.txt
│       ├── CHANGELOG.rst
│       ├── CMakeLists.txt
│       ├── CONTRIBUTING.md
│       ├── Kconfig.projbuild
│       ├── LICENSE
│       ├── NOTICE
│       ├── README.md
│       ├── colcon.meta
│       ├── docker
│       │   ├── Dockerfile
│       │   └── install_micro_ros_deps_script.sh
│       ├── esp32_toolchain.cmake.in
│       ├── examples
│       │   ├── addtwoints_server
│       │   │   ├── CMakeLists.txt
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── handle_static_types
│       │   │   ├── CMakeLists.txt
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── int32_publisher
│       │   │   ├── CMakeLists.txt
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── int32_publisher_custom_transport
│       │   │   ├── CMakeLists.txt
│       │   │   ├── app-colcon.meta
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   ├── esp32_serial_transport.c
│       │   │   │   ├── esp32_serial_transport.h
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── int32_publisher_custom_transport_usbcdc
│       │   │   ├── CMakeLists.txt
│       │   │   ├── README.md
│       │   │   ├── app-colcon.meta
│       │   │   ├── components
│       │   │   │   ├── esp32s2_usbcdc_logging
│       │   │   │   │   ├── CMakeLists.txt
│       │   │   │   │   ├── esp32s2_usbcdc_logging.c
│       │   │   │   │   ├── esp32s2_usbcdc_logging.h
│       │   │   │   │   └── idf_component.yml
│       │   │   │   └── esp32s2_usbcdc_transport
│       │   │   │       ├── CMakeLists.txt
│       │   │   │       ├── esp32s2_usbcdc_transport.c
│       │   │   │       ├── esp32s2_usbcdc_transport.h
│       │   │   │       └── idf_component.yml
│       │   │   ├── dependencies.lock
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── int32_publisher_embeddedrtps
│       │   │   ├── CMakeLists.txt
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── int32_sub_pub
│       │   │   ├── CMakeLists.txt
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── low_consumption
│       │   │   ├── CMakeLists.txt
│       │   │   ├── README.md
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── multithread_publisher
│       │   │   ├── CMakeLists.txt
│       │   │   ├── app-colcon.meta
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   ├── parameters
│       │   │   ├── CMakeLists.txt
│       │   │   ├── app-colcon.meta
│       │   │   ├── main
│       │   │   │   ├── CMakeLists.txt
│       │   │   │   ├── Kconfig.projbuild
│       │   │   │   ├── component.mk
│       │   │   │   └── main.c
│       │   │   └── sdkconfig.defaults
│       │   └── ping_pong
│       │       ├── CMakeLists.txt
│       │       ├── main
│       │       │   ├── CMakeLists.txt
│       │       │   ├── Kconfig.projbuild
│       │       │   ├── component.mk
│       │       │   └── main.c
│       │       └── sdkconfig.defaults
│       ├── extra_packages
│       │   └── README.md
│       ├── include_override
│       │   ├── FreeRTOS.h
│       │   ├── assert.h
│       │   ├── esp_macros.h
│       │   └── semphr.h
│       ├── libmicroros.mk
│       ├── network_interfaces
│       │   ├── uros_ethernet_netif.c
│       │   ├── uros_network_interfaces.h
│       │   └── uros_wlan_netif.c
│       └── package.xml
├── main
│   ├── CmakeLists.txt
│   └── main.c
└── sdkconfig.defaults

34 directories, 109 files
```
内容已经写好了。






