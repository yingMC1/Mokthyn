# EGE（Easy Graphics Engine）函数总表

下面按功能分类整理 `graphics.h` / `ege.h` 中常用与重要的 API，方便快速查阅和使用。

---

## 一、窗口与系统控制

| 函数                                                                            | 参数          | 作用                     |
| :------------------------------------------------------------------------------ | :------------ | :----------------------- |
| `setcodepage(unsigned int codepage)`                                            | 代码页        | 设置字符编码             |
| `getcodepage()`                                                                 | 无            | 获取当前代码页           |
| `setunicodecharmessage(bool enable)`                                            | 开关          | 控制 Unicode 字符消息    |
| `getunicodecharmessage()`                                                       | 无            | 查询 Unicode 消息状态    |
| `setinitmode(initmode_flag mode, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT)` | 模式/位置     | 设置窗口初始化参数       |
| `getinitmode()`                                                                 | 无            | 获取当前初始化模式       |
| `initgraph(int width, int height, initmode_flag mode)`                          | 宽、高、模式  | 初始化图形窗口           |
| `initgraph(int width, int height)`                                              | 宽、高        | 初始化图形窗口（简化版） |
| `initgraph(int *graphdriver, int *graphmode, const char *pathtodriver)`         | 驱动参数      | BGI 兼容初始化           |
| `closegraph()`                                                                  | 无            | 关闭图形系统             |
| `is_run()`                                                                      | 无            | 判断图形环境是否运行     |
| `setcaption(const char* caption)`                                               | 标题          | 设置窗口标题（ANSI）     |
| `setcaption(const wchar_t* caption)`                                            | 标题          | 设置窗口标题（Unicode）  |
| `seticon(int icon_id)`                                                          | 图标 ID       | 设置窗口图标             |
| `attachHWND(HWND hWnd)`                                                         | 窗口句柄      | 绑定现有窗口             |
| `showwindow()`                                                                  | 无            | 显示窗口                 |
| `hidewindow()`                                                                  | 无            | 隐藏窗口                 |
| `movewindow(int x, int y, bool redraw = true)`                                  | 坐标          | 移动窗口                 |
| `resizewindow(int width, int height)`                                           | 宽、高        | 调整窗口大小             |
| `flushwindow()`                                                                 | 无            | 刷新窗口显示             |
| `setrendermode(rendermode_e mode)`                                              | 模式          | 设置渲染模式             |
| `gettarget()`                                                                   | 无            | 获取当前绘图目标         |
| `settarget(PIMAGE pbuf)`                                                        | 图片对象      | 设置绘图目标             |
| `cleardevice(PIMAGE pimg = NULL)`                                               | 目标图片      | 清屏/清空图像            |
| `getviewport(...)`                                                              | 左上右下等    | 获取视口设置             |
| `setviewport(...)`                                                              | 左上右下等    | 设置视口                 |
| `clearviewport(PIMAGE pimg = NULL)`                                             | 目标图片      | 清空视口                 |
| `setactivepage(int page)`                                                       | 页号          | 设置活动页（已弃用）     |
| `setvisualpage(int page)`                                                       | 页号          | 设置显示页（已弃用）     |
| `swappage()`                                                                    | 无            | 交换页（已弃用）         |
| `window_getviewport(...)`                                                       | 视口结构/坐标 | 获取窗口视口             |
| `window_setviewport(...)`                                                       | 左上右下      | 设置窗口视口             |

---

## 二、颜色、混合与转换

| 函数                                                                 | 参数               | 作用              |
| :------------------------------------------------------------------- | :----------------- | :---------------- |
| `rgb2gray(color_t rgb)`                                              | RGB 颜色           | RGB 转灰度        |
| `rgb2hsl(color_t rgb, float* H, float* S, float* L)`                 | RGB 颜色，输出 HSL | 颜色转换          |
| `rgb2hsv(color_t rgb, float* H, float* S, float* V)`                 | RGB 颜色，输出 HSV | 颜色转换          |
| `hsl2rgb(float H, float S, float L)`                                 | HSL 参数           | 转回 RGB          |
| `hsv2rgb(float H, float S, float V)`                                 | HSV 参数           | 转回 RGB          |
| `colorblend(color_t dst, color_t src, unsigned char alpha)`          | 目标/源/透明度     | 颜色混合          |
| `colorblend_f(...)`                                                  | 目标/源/透明度     | 快速版本颜色混合  |
| `alphablend(color_t dst, color_t src)`                               | 目标/源            | alpha 混合        |
| `alphablend(color_t dst, color_t src, unsigned char srcAlphaFactor)` | 目标/源/因子       | 带因子 alpha 混合 |
| `alphablend_premultiplied(...)`                                      | 预乘 alpha         | 预乘 alpha 混合   |
| `color_unpremultiply(color_t color)`                                 | 颜色               | 还原非预乘 alpha  |
| `color_premultiply(color_t color)`                                   | 颜色               | 转为预乘 alpha    |
| `image_convertcolor(PIMAGE pimg, color_type src, color_type dst)`    | 图片、源/目的色型  | 转换图像颜色类型  |

### 常用颜色宏

| 宏                    | 作用           |
| :-------------------- | :------------- |
| `EGERGB(r, g, b)`     | 生成 RGB 颜色  |
| `EGERGBA(r, g, b, a)` | 生成 RGBA 颜色 |
| `EGEARGB(a, r, g, b)` | 生成 ARGB 颜色 |
| `EGEGRAY(gray)`       | 灰度色         |
| `EGEGET_R(c)`         | 取 R 分量      |
| `EGEGET_G(c)`         | 取 G 分量      |
| `EGEGET_B(c)`         | 取 B 分量      |
| `EGEGET_A(c)`         | 取 A 分量      |

### 预定义颜色常量

`ALICEBLUE`、`BLACK`、`BLUE`、`RED`、`GREEN`、`WHITE`、`YELLOW`、`MAGENTA` 等，EGE 提供了大量 Web 颜色常量。

---

## 三、线条、填充与样式

| 函数                                                           | 参数           | 作用                |
| :------------------------------------------------------------- | :------------- | :------------------ |
| `setlinewidth(float width, PIMAGE pimg = NULL)`                | 宽度           | 设置线宽            |
| `getlinestyle(...)`                                            | 输出参数       | 获取线样式          |
| `setlinestyle(...)`                                            | 样式/图案/粗细 | 设置线样式          |
| `setlinecap(...)`                                              | 端点样式       | 设置线帽样式        |
| `getlinecap(...)`                                              | 输出参数       | 获取线帽样式        |
| `setlinejoin(...)`                                             | 接头样式       | 设置线接头样式      |
| `getlinejoin(...)`                                             | 输出参数       | 获取线接头样式      |
| `setfillstyle(int pattern, color_t color, PIMAGE pimg = NULL)` | 填充方式/颜色  | 设置填充样式        |
| `setwritemode(int mode, PIMAGE pimg = NULL)`                   | 写入模式       | 设置写入模式        |
| `setcolor(color_t color, PIMAGE pimg = NULL)`                  | 颜色           | 设置当前前景色      |
| `setlinecolor(color_t color, PIMAGE pimg = NULL)`              | 颜色           | 设置线条颜色        |
| `setfillcolor(color_t color, PIMAGE pimg = NULL)`              | 颜色           | 设置填充颜色        |
| `setbkcolor(color_t color, PIMAGE pimg = NULL)`                | 颜色           | 设置背景色          |
| `setbkcolor_f(color_t color, PIMAGE pimg = NULL)`              | 颜色           | 半透明背景色设置    |
| `settextcolor(color_t color, PIMAGE pimg = NULL)`              | 颜色           | 设置文本颜色        |
| `setfontbkcolor(color_t color, PIMAGE pimg = NULL)`            | 颜色           | 设置字体背景色      |
| `setbkmode(int bkMode, PIMAGE pimg = NULL)`                    | 背景模式       | 设置透明/不透明背景 |

### 线型与填充枚举

- `SOLID_LINE`, `CENTER_LINE`, `DOTTED_LINE`, `DASHED_LINE`, `NULL_LINE`, `USERBIT_LINE`
- `EMPTY_FILL`, `SOLID_FILL`, `LINE_FILL`, `LTSLASH_FILL`, `SLASH_FILL`, `BKSLASH_FILL`, `HATCH_FILL`, `XHATCH_FILL`, `INTERLEAVE_FILL`, `WIDE_DOT_FILL`, `CLOSE_DOT_FILL`, `USER_FILL`

---

## 四、像素与基本绘制

| 函数                                                                 | 参数              | 作用                 |
| :------------------------------------------------------------------- | :---------------- | :------------------- |
| `getpixel(int x, int y, PCIMAGE pimg = NULL)`                        | 坐标              | 获取像素颜色         |
| `putpixel(int x, int y, color_t color, PIMAGE pimg = NULL)`          | 坐标/颜色         | 设定像素颜色         |
| `getpixel_f(int x, int y, PCIMAGE pimg = NULL)`                      | 坐标              | 快速获取像素         |
| `putpixel_f(int x, int y, color_t color, PIMAGE pimg = NULL)`        | 坐标/颜色         | 快速设置像素         |
| `putpixels(int numOfPoints, const int* points, PIMAGE pimg = NULL)`  | 点阵数组          | 批量设置像素         |
| `putpixels_f(...)`                                                   | 点阵数组          | 快速批量设置像素     |
| `putpixel_withalpha(...)`                                            | 坐标/颜色         | 带 alpha 设置像素    |
| `putpixel_savealpha(...)`                                            | 坐标/颜色         | 保留 alpha 设置像素  |
| `putpixel_alphablend(...)`                                           | 坐标/颜色         | alpha 混合绘制像素   |
| `moveto(int x, int y, PIMAGE pimg = NULL)`                           | 坐标              | 移动当前绘图位置     |
| `moverel(int dx, int dy, PIMAGE pimg = NULL)`                        | 偏移量            | 相对移动             |
| `line(int x1, int y1, int x2, int y2, PIMAGE pimg = NULL)`           | 起止点            | 画直线               |
| `line_f(float x1, float y1, float x2, float y2, PIMAGE pimg = NULL)` | 浮点直线          | 浮点版本直线         |
| `lineto(int x, int y, PIMAGE pimg = NULL)`                           | 终点              | 从当前点到目标点画线 |
| `lineto_f(float x, float y, PIMAGE pimg = NULL)`                     | 浮点终点          | 浮点版线段绘制       |
| `linerel(int dx, int dy, PIMAGE pimg = NULL)`                        | 偏移量            | 相对画线             |
| `linerel_f(float dx, float dy, PIMAGE pimg = NULL)`                  | 浮点偏移          | 相对浮点画线         |
| `ellipse(...)`                                                       | 圆心/角度/半径    | 画椭圆弧             |
| `sector(...)`                                                        | 圆心/角度/半径    | 画扇形               |
| `pie(...)`                                                           | 圆心/角度/半径    | 画饼图               |
| `fillpie(...)`                                                       | 圆心/角度/半径    | 填充扇形             |
| `solidpie(...)`                                                      | 圆心/角度/半径    | 实心扇形             |
| `arc(...)`                                                           | 圆心/角度/半径    | 画圆弧               |
| `pieslice(...)`                                                      | 圆心/角度/半径    | 画圆形扇形           |
| `fillellipse(...)`                                                   | 圆心/半径         | 填充椭圆             |
| `solidellipse(...)`                                                  | 圆心/半径         | 实心椭圆             |
| `circle(...)`                                                        | 圆心/半径         | 画圆                 |
| `fillcircle(...)`                                                    | 圆心/半径         | 填充圆               |
| `solidcircle(...)`                                                   | 圆心/半径         | 实心圆               |
| `bar3d(...)`                                                         | 左上右下/深度     | 3D 柱状图            |
| `bar(...)`                                                           | 左上右下          | 填充矩形             |
| `rectangle(...)`                                                     | 左上右下          | 空心矩形             |
| `fillrect(...)`                                                      | 左上右下          | 填充矩形             |
| `solidrect(...)`                                                     | 左上右下          | 实心矩形             |
| `roundrect(...)`                                                     | 左上右下/圆角半径 | 圆角矩形             |
| `fillroundrect(...)`                                                 | 左上右下/圆角半径 | 填充圆角矩形         |
| `solidroundrect(...)`                                                | 左上右下/圆角半径 | 实心圆角矩形         |
| `drawpoly(...)`                                                      | 点数组            | 画折线               |
| `polyline(...)`                                                      | 点数组            | 多段线               |
| `polygon(...)`                                                       | 点数组            | 空心多边形           |
| `fillpoly(...)`                                                      | 点数组            | 填充多边形           |
| `solidpoly(...)`                                                     | 点数组            | 实心多边形           |
| `fillpoly_gradient(...)`                                             | 颜色点数组        | 渐变填充多边形       |
| `drawlines(...)`                                                     | 多线段数组        | 画多条线段           |
| `drawbezier(...)`                                                    | 控制点            | 画贝塞尔曲线         |
| `floodfill(...)`                                                     | 起点/边界色       | 泛洪填充             |
| `floodfillsurface(...)`                                              | 起点/填充色       | 区域填充             |

---

## 五、GDI+ 增强版函数

> 这些函数通常在 `#define EGE_GDIPLUS` 后启用，提供更高级的绘制能力。

| 函数                                                                      | 参数             | 作用                  |
| :------------------------------------------------------------------------ | :--------------- | :-------------------- |
| `ege_enable_aa(bool enable, PIMAGE pimg = NULL)`                          | 开关             | 开启/关闭抗锯齿       |
| `ege_setalpha(int alpha, PIMAGE pimg = NULL)`                             | 透明度           | 设置全局 alpha        |
| `ege_line(float x1, float y1, float x2, float y2, PIMAGE pimg = NULL)`    | 浮点坐标         | 改进版直线            |
| `ege_drawpoly(...)`                                                       | 顶点数组         | 画多边形              |
| `ege_polyline(...)`                                                       | 顶点数组         | 画多段线              |
| `ege_polygon(...)`                                                        | 顶点数组         | 画空心多边形          |
| `ege_fillpoly(...)`                                                       | 顶点数组         | 填充多边形            |
| `ege_bezier(...)`                                                         | 控制点           | 贝塞尔曲线            |
| `ege_drawbezier(...)`                                                     | 控制点           | 同上，别名版本        |
| `ege_drawcurve(...)`                                                      | 控制点/张力      | 绘制样条曲线          |
| `ege_drawclosedcurve(...)`                                                | 控制点/张力      | 绘制闭合样条          |
| `ege_fillclosedcurve(...)`                                                | 控制点/张力      | 填充闭合样条          |
| `ege_rectangle(float x, float y, float w, float h, PIMAGE pimg = NULL)`   | 左上/宽高        | 浮点矩形              |
| `ege_fillrect(float x, float y, float w, float h, PIMAGE pimg = NULL)`    | 左上/宽高        | 浮点填充矩形          |
| `ege_circle(float x, float y, float radius, PIMAGE pimg = NULL)`          | 圆心/半径        | 浮点圆                |
| `ege_fillcircle(float x, float y, float radius, PIMAGE pimg = NULL)`      | 圆心/半径        | 浮点填充圆            |
| `ege_ellipse(...)`                                                        | 左上/宽高        | 浮点椭圆              |
| `ege_fillellipse(...)`                                                    | 左上/宽高        | 浮点填充椭圆          |
| `ege_arc(...)`                                                            | 左上/宽高/角度   | 浮点弧形              |
| `ege_pie(...)`                                                            | 左上/宽高/角度   | 浮点饼图              |
| `ege_fillpie(...)`                                                        | 左上/宽高/角度   | 浮点填充饼图          |
| `ege_roundrect(...)`                                                      | 左上/宽高/半径   | 浮点圆角矩形          |
| `ege_fillroundrect(...)`                                                  | 左上/宽高/半径   | 浮点填充圆角矩形      |
| `ege_setpattern_none(PIMAGE pimg = NULL)`                                 | 无               | 清除填充纹理          |
| `ege_setpattern_lineargradient(...)`                                      | 起止点/颜色      | 设置线性渐变填充      |
| `ege_setpattern_pathgradient(...)`                                        | 中心/边界点/颜色 | 设置路径渐变填充      |
| `ege_setpattern_ellipsegradient(...)`                                     | 中心/椭圆/颜色   | 设置椭圆渐变填充      |
| `ege_setpattern_texture(...)`                                             | 纹理图片         | 设置纹理填充          |
| `ege_drawtext(const char* text, float x, float y, PIMAGE pimg = NULL)`    | 文本             | 浮点文本绘制          |
| `ege_drawtext(const wchar_t* text, float x, float y, PIMAGE pimg = NULL)` | Unicode 文本     | 浮点 Unicode 文本绘制 |
| `ege_gentexture(bool generate, PIMAGE pimg = NULL)`                       | 开关             | 生成纹理              |
| `ege_puttexture(...)`                                                     | 图片/区域        | 贴图绘制              |
| `ege_drawimage(...)`                                                      | 图片/区域        | 绘制图像              |
| `ege_drawpath(...)`                                                       | 路径对象         | 绘制路径              |
| `ege_fillpath(...)`                                                       | 路径对象         | 填充路径              |

---

## 六、路径与变换

| 函数                                                                  | 参数             | 作用               |
| :-------------------------------------------------------------------- | :--------------- | :----------------- |
| `ege_path_create()`                                                   | 无               | 创建路径对象       |
| `ege_path_createfrom(...)`                                            | 点数组/类型数组  | 从点数据创建路径   |
| `ege_path_clone(...)`                                                 | 路径对象         | 克隆路径           |
| `ege_path_destroy(...)`                                               | 路径对象         | 销毁路径           |
| `ege_path_start(...)`                                                 | 路径对象         | 开始新子路径       |
| `ege_path_close(...)`                                                 | 路径对象         | 闭合当前子路径     |
| `ege_path_closeall(...)`                                              | 路径对象         | 闭合所有子路径     |
| `ege_path_setfillmode(...)`                                           | 路径/填充模式    | 设置填充模式       |
| `ege_path_reset(...)`                                                 | 路径对象         | 清空路径           |
| `ege_path_reverse(...)`                                               | 路径对象         | 反转路径方向       |
| `ege_path_widen(...)`                                                 | 路径/线宽        | 生成路径轮廓       |
| `ege_path_flatten(...)`                                               | 路径对象         | 展平曲线           |
| `ege_path_warp(...)`                                                  | 路径/控制点/区域 | 扭曲路径           |
| `ege_path_outline(...)`                                               | 路径对象         | 生成轮廓           |
| `ege_path_inpath(...)`                                                | 路径/点          | 判断点是否在路径内 |
| `ege_path_instroke(...)`                                              | 路径/点          | 判断点是否在轮廓上 |
| `ege_path_lastpoint(...)`                                             | 路径对象         | 获取最后一个点     |
| `ege_path_pointcount(...)`                                            | 路径对象         | 获取点数量         |
| `ege_path_getbounds(...)`                                             | 路径对象         | 获取边界矩形       |
| `ege_path_getpathpoints(...)`                                         | 路径对象         | 获取所有点         |
| `ege_path_getpathtypes(...)`                                          | 路径对象         | 获取所有点类型     |
| `ege_path_transform(...)`                                             | 路径/矩阵        | 变换路径           |
| `ege_path_addpath(...)`                                               | 源路径/目标路径  | 合并路径           |
| `ege_path_addline(...)`                                               | 路径/线段        | 添加线段           |
| `ege_path_addarc(...)`                                                | 路径/椭圆弧      | 添加弧线           |
| `ege_path_addpolyline(...)`                                           | 路径/点数组      | 添加折线           |
| `ege_path_addbezier(...)`                                             | 路径/控制点      | 添加贝塞尔曲线     |
| `ege_path_addcurve(...)`                                              | 路径/控制点      | 添加样条曲线       |
| `ege_path_addcircle(...)`                                             | 路径/圆          | 添加圆             |
| `ege_path_addrect(...)`                                               | 路径/矩形        | 添加矩形           |
| `ege_path_addellipse(...)`                                            | 路径/椭圆        | 添加椭圆           |
| `ege_path_addpie(...)`                                                | 路径/扇形        | 添加扇形           |
| `ege_path_addtext(...)`                                               | 路径/文本        | 添加文本路径       |
| `ege_path_addpolygon(...)`                                            | 路径/点数组      | 添加多边形         |
| `ege_path_addclosedcurve(...)`                                        | 路径/控制点      | 添加闭合样条       |
| `ege_transform_rotate(float angle, PIMAGE pimg = NULL)`               | 角度             | 旋转变换           |
| `ege_transform_translate(float x, float y, PIMAGE pimg = NULL)`       | 位移             | 平移变换           |
| `ege_transform_scale(float xScale, float yScale, PIMAGE pimg = NULL)` | 缩放因子         | 缩放变换           |
| `ege_transform_reset(PIMAGE pimg = NULL)`                             | 无               | 重置变换矩阵       |
| `ege_get_transform(...)`                                              | 输出矩阵         | 获取当前变换矩阵   |
| `ege_set_transform(...)`                                              | 矩阵             | 设置变换矩阵       |
| `ege_transform_calc(...)`                                             | 点/坐标          | 计算变换后坐标     |

---

## 七、文本输出与字体设置

| 函数                                                                                                                                                                 | 参数              | 作用                      |
| :------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :---------------- | :------------------------ |
| `settextjustify(int horiz, int vert, PIMAGE pimg = NULL)`                                                                                                            | 对齐方式          | 设置文本对齐              |
| `setfont(int height, int width, const char* typeface, PIMAGE pimg = NULL)`                                                                                           | 字高/字宽/字体    | 简化设置字体              |
| `setfont(int height, int width, const wchar_t* typeface, PIMAGE pimg = NULL)`                                                                                        | 字高/字宽/字体    | Unicode 字体设置          |
| `setfont(int height, int width, const char* typeface, int escapement, int orientation, int weight, bool italic, bool underline, bool strikeOut, PIMAGE pimg = NULL)` | 完整参数          | 设置完整字体              |
| `setfont(const LOGFONTW *font, PIMAGE pimg = NULL)`                                                                                                                  | LOGFONTW 指针     | 设置字体结构              |
| `getfont(LOGFONTW *font, PCIMAGE pimg = NULL)`                                                                                                                       | 输出结构体        | 获取当前字体              |
| `outtext(const char* text, PIMAGE pimg = NULL)`                                                                                                                      | 文本              | 在当前位置输出文本        |
| `outtext(const wchar_t* text, PIMAGE pimg = NULL)`                                                                                                                   | Unicode 文本      | 输出宽字符文本            |
| `outtextxy(int x, int y, const char* text, PIMAGE pimg = NULL)`                                                                                                      | 坐标/文本         | 指定位置输出文本          |
| `outtextxy(int x, int y, const wchar_t* text, PIMAGE pimg = NULL)`                                                                                                   | 坐标/Unicode 文本 | 指定位置输出 Unicode 文本 |
| `xyprintf(int x, int y, const char* format, ...)`                                                                                                                    | 坐标/格式化字符串 | 格式化输出                |
| `xyprintf(int x, int y, const wchar_t* format, ...)`                                                                                                                 | 坐标/格式化字符串 | Unicode 格式化输出        |
| `outtextrect(int x, int y, int w, int h, const char* text, PIMAGE pimg = NULL)`                                                                                      | 区域/文本         | 区域内文本输出            |
| `outtextrect(int x, int y, int w, int h, const wchar_t* text, PIMAGE pimg = NULL)`                                                                                   | 区域/Unicode 文本 | 区域内 Unicode 文本输出   |
| `rectprintf(...)`                                                                                                                                                    | 区域/格式化字符串 | 区域内格式化输出          |
| `textwidth(const char* text, PCIMAGE pimg = NULL)`                                                                                                                   | 文本              | 获取文本宽度              |
| `textwidth(const wchar_t* text, PCIMAGE pimg = NULL)`                                                                                                                | Unicode 文本      | 获取宽字符文本宽度        |
| `textheight(...)`                                                                                                                                                    | 文本              | 获取文本高度              |
| `measuretext(...)`                                                                                                                                                   | 文本              | 精确测量文本宽高          |
| `ege_outtextxy(float x, float y, const char* text, PIMAGE pimg = NULL)`                                                                                              | 浮点坐标          | GDI+ 精确文本输出         |
| `ege_outtextxy(float x, float y, const wchar_t* text, PIMAGE pimg = NULL)`                                                                                           | 浮点坐标/Unicode  | GDI+ Unicode 文本输出     |
| `ege_xyprintf(float x, float y, const char* format, ...)`                                                                                                            | 浮点坐标          | GDI+ 格式化输出           |
| `ege_xyprintf(float x, float y, const wchar_t* format, ...)`                                                                                                         | 浮点坐标/Unicode  | GDI+ Unicode 格式化输出   |

### 文本对齐枚举

- `LEFT_TEXT`, `CENTER_TEXT`, `RIGHT_TEXT`
- `TOP_TEXT`, `BOTTOM_TEXT`

---

## 八、图像对象与加载/保存

| 函数                                                                                        | 参数          | 作用               |
| :------------------------------------------------------------------------------------------ | :------------ | :----------------- |
| `newimage()`                                                                                | 无            | 创建空图像         |
| `newimage(int width, int height)`                                                           | 宽高          | 创建指定大小图像   |
| `delimage(PCIMAGE pimg)`                                                                    | 图片对象      | 销毁图像           |
| `getbuffer(PIMAGE pimg)`                                                                    | 图片对象      | 获取像素缓冲区     |
| `getbuffer(PCIMAGE pimg)`                                                                   | 图片对象      | 获取只读像素缓冲区 |
| `resize_f(PIMAGE pimg, int width, int height)`                                              | 图片对象/宽高 | 快速重设大小       |
| `resize(PIMAGE pimg, int width, int height)`                                                | 图片对象/宽高 | 标准重设大小       |
| `getimage(PIMAGE imgDest, int xSrc, int ySrc, int widthSrc, int heightSrc)`                 | 截图区域      | 从窗口抓图         |
| `getimage(PIMAGE imgDest, PCIMAGE imgSrc, int xSrc, int ySrc, int widthSrc, int heightSrc)` | 来源图像区域  | 从图像中复制区域   |
| `getimage(PIMAGE imgDest, const char* imageFile, int zoomWidth = 0, int zoomHeight = 0)`    | 文件          | 加载图片           |
| `getimage(PIMAGE imgDest, const wchar_t* imageFile, int zoomWidth = 0, int zoomHeight = 0)` | 文件          | Unicode 文件加载   |
| `getimage_pngfile(PIMAGE pimg, const char* filename)`                                       | PNG 文件      | 专门加载 PNG       |
| `saveimage(PCIMAGE pimg, const char* filename, bool withAlphaChannel = false)`              | 保存路径      | 图片保存           |
| `saveimage(PCIMAGE pimg, const wchar_t* filename, bool withAlphaChannel = false)`           | 保存路径      | Unicode 图片保存   |
| `savepng(...)`                                                                              | 保存 PNG      | 强制保存 PNG       |
| `savebmp(...)`                                                                              | 保存 BMP      | 强制保存 BMP       |

---

## 九、图像绘制与透明处理

| 函数                                                                                    | 参数                | 作用                      |
| :-------------------------------------------------------------------------------------- | :------------------ | :------------------------ |
| `putimage(int x, int y, PCIMAGE pimg, DWORD dwRop = SRCCOPY)`                           | 坐标/图片/操作码    | 基本图像绘制              |
| `putimage(PIMAGE imgDest, int xDest, int yDest, PCIMAGE imgSrc, DWORD dwRop = SRCCOPY)` | 目标图像/源图像     | 图片到图片绘制            |
| `putimage_transparent(...)`                                                             | 透明色              | 指定颜色透明处理          |
| `putimage_alphablend(...)`                                                              | alpha               | alpha 混合绘制            |
| `putimage_alphatransparent(...)`                                                        | transparent + alpha | 透明+alpha 结合           |
| `putimage_withalpha(...)`                                                               | 使用图像自身 alpha  | 按 alpha 通道绘制         |
| `putimage_alphafilter(...)`                                                             | alpha mask 图       | 用另一张图作为 alpha 蒙版 |
| `putimage_rotate(...)`                                                                  | 旋转角度            | 旋转绘制                  |
| `putimage_rotatezoom(...)`                                                              | 旋转+缩放           | 旋转缩放绘制              |
| `putimage_rotatetransparent(...)`                                                       | 透明色+旋转         | 透明色旋转绘制            |
| `imagefilter_blurring(...)`                                                             | 模糊强度            | 图像模糊处理              |

---

## 十、输入处理：键盘与鼠标

### 键盘

| 函数                  | 参数 | 作用                 |
| :-------------------- | :--- | :------------------- |
| `kbmsg()`             | 无   | 检查键盘消息是否存在 |
| `getkey()`            | 无   | 获取键盘消息         |
| `keystate(int key)`   | 键码 | 检查某键是否按下     |
| `keypress(int key)`   | 键码 | 获取按键次数         |
| `keyrelease(int key)` | 键码 | 获取键释放次数       |
| `keyrepeat(int key)`  | 键码 | 获取重复按键次数     |
| `flushkey()`          | 无   | 清空键盘消息队列     |
| `ege_getch()`         | 无   | EGE 内部 getch       |
| `ege_kbhit()`         | 无   | EGE 内部 kbhit       |
| `getch()`             | 无   | 获取字符键输入       |
| `kbhit()`             | 无   | 检查是否有字符输入   |

### 鼠标

| 函数                       | 参数     | 作用                 |
| :------------------------- | :------- | :------------------- |
| `mousemsg()`               | 无       | 检查鼠标消息         |
| `getmouse()`               | 无       | 获取鼠标消息         |
| `GetMouseMsg()`            | 无       | 兼容旧版鼠标消息获取 |
| `flushmouse()`             | 无       | 清空鼠标消息队列     |
| `showmouse(int bShow)`     | 是否显示 | 显示/隐藏鼠标        |
| `mousepos(int *x, int *y)` | 输出坐标 | 获取鼠标当前位置     |

---

## 十一、时间与随机

| 函数                           | 参数 | 作用                 |
| :----------------------------- | :--- | :------------------- |
| `ege_sleep(long ms)`           | 毫秒 | 高精度睡眠           |
| `delay(long ms)`               | 毫秒 | 延迟                 |
| `delay_ms(long ms)`            | 毫秒 | 毫秒级延迟           |
| `api_sleep(long ms)`           | 毫秒 | API 延迟             |
| `delay_fps(int fps)`           | fps  | 按帧率延时           |
| `delay_jfps(int fps)`          | fps  | 更精确的帧率延时     |
| `fclock()`                     | 无   | 获取高精度时钟       |
| `randomize()`                  | 无   | 初始化随机数种子     |
| `randomize(unsigned int seed)` | 种子 | 指定随机种子         |
| `random(unsigned int n = 0)`   | 上限 | 获取随机整数         |
| `randomf()`                    | 无   | 获取随机浮点数 [0,1) |

---

## 十二、控制台函数

| 函数              | 参数 | 作用           |
| :---------------- | :--- | :------------- |
| `init_console()`  | 无   | 初始化控制台   |
| `clear_console()` | 无   | 清屏控制台     |
| `show_console()`  | 无   | 显示控制台     |
| `hide_console()`  | 无   | 隐藏控制台     |
| `close_console()` | 无   | 关闭控制台     |
| `getch_console()` | 无   | 控制台输入字符 |
| `kbhit_console()` | 无   | 检查控制台输入 |

---

## 十三、音频与音乐

| 类/函数                         | 作用         |
| :------------------------------ | :----------- |
| `class MUSIC`                   | 音乐播放类   |
| `MUSIC::OpenFile(...)`          | 打开音乐文件 |
| `MUSIC::Play(...)`              | 播放         |
| `MUSIC::RepeatPlay(...)`        | 循环播放     |
| `MUSIC::Pause()`                | 暂停         |
| `MUSIC::Seek(...)`              | 快进/定位    |
| `MUSIC::SetVolume(float value)` | 调整音量     |
| `MUSIC::Close()`                | 关闭音乐     |
| `MUSIC::Stop()`                 | 停止播放     |
| `MUSIC::GetPosition()`          | 获取播放位置 |
| `MUSIC::GetLength()`            | 获取时长     |
| `MUSIC::GetPlayStatus()`        | 获取状态     |

---

## 十四、窗口句柄与系统信息

| 函数                                     | 作用             |
| :--------------------------------------- | :--------------- |
| `HWND EGEAPI getHWnd()`                  | 获取窗口句柄     |
| `HINSTANCE EGEAPI getHInstance()`        | 获取实例句柄     |
| `HDC EGEAPI getHDC(PCIMAGE pimg = NULL)` | 获取设备上下文   |
| `PVOID EGEAPI getProcfunc()`             | 获取过程函数指针 |
| `long EGEAPI getGraphicsVer()`           | 获取 EGE 版本号  |
| `float EGEAPI getfps()`                  | 获取当前 FPS     |

---

## 十五、输入框与杂项

| 函数                                    | 参数                  | 作用             |
| :-------------------------------------- | :-------------------- | :--------------- |
| `inputbox_getline(...)`                 | 标题/提示/缓冲区/长度 | 输入对话框       |
| `SetCloseHandler(LPCALLBACK_PROC func)` | 回调函数              | 设置关闭事件处理 |
| `ege_compress_bound(...)`               | 数据长度              | 压缩上限估算     |
| `ege_compress(...)`                     | 压缩算法              | 数据压缩         |
| `ege_compress2(...)`                    | 压缩算法/级别         | 二级压缩接口     |
| `ege_uncompress(...)`                   | 解压缩                | 数据解压         |
| `ege_uncompress_size(...)`              | 压缩数据              | 获取解压后大小   |

---

## 十六、常见工作流建议

### 1）基础绘图

```cpp
initgraph(800, 600);
setbkcolor(WHITE);
cleardevice();
setcolor(BLUE);
setfillcolor(YELLOW);
rectangle(100, 100, 300, 250);
bar(350, 100, 550, 250);
setfont(30, 0, "Consolas");
outtextxy(100, 300, "Hello EGE");
getch();
closegraph();
```

### 2）图像加载与绘制

```cpp
initgraph(800, 600);
PIMAGE img = newimage();
getimage(img, "demo.png");
putimage(100, 100, img);
getch();
closegraph();
```

### 3）键盘输入

```cpp
initgraph(800, 600);
while (1) {
    if (kbmsg()) {
        key_msg k = getkey();
        if (k.key == KEY_ENTER) break;
    }
}
closegraph();
```

---

## 结语

EGE 的 API 非常丰富，且有大量重载版本；因此在实际开发中，通常使用的核心是：

- `initgraph` / `closegraph`
- `setcolor` / `setfillcolor` / `setfont`
- `line` / `circle` / `rectangle` / `bar`
- `putpixel` / `getpixel`
- `outtextxy` / `xyprintf`
- `getkey` / `getmouse`
- `newimage` / `getimage` / `putimage`

如果你需要，我还可以继续整理成以下两种版本：

1. “按字母序的函数索引版”
2. “按常用场景的速查版（绘图/文本/图像/输入）”

你要的话，我可以直接继续补一份更适合复制到代码项目里的精简版。
