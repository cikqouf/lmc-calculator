用到的依赖: 

1. vulkan: vulkan_raii.hpp, raii 库在句柄管理上相比 hpp 和 c 库方便很多

2. glfw: 用于创建 surface 和 window, 也可以直接用 win32 api 代替

3. stb_image 和 stb_truetype: 用于字体贴图生成
