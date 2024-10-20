idf_component_register(SRCS "main.cpp" "ppi2c/i2c_slave_driver.c" "ppi2c/pp_handler.cpp"
                       INCLUDE_DIRS "." "../../uart/build" "./ppi2c"
                       REQUIRES driver esp_driver_i2c)
