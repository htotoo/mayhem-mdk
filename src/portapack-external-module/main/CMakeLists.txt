idf_component_register(SRCS "main.cpp" "i2c_slave_driver.c"
                       INCLUDE_DIRS "." "../../uart/build"
                       REQUIRES driver esp_driver_i2c)
