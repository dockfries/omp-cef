# omp-cef

[English](README.md) | **简体中文**

![Version](https://img.shields.io/github/v/release/dockfries/omp-cef?include_prereleases&label=version)
![License](https://img.shields.io/github/license/dockfries/omp-cef)
[![Wiki](https://img.shields.io/badge/docs-wiki-blue)](https://github.com/dockfries/omp-cef/wiki)
![CEF](https://img.shields.io/badge/CEF-148.0.10-blue)
![SA--MP](https://img.shields.io/badge/SA--MP-0.3.7%20%7C%200.3.DL-orange)
![open.mp](https://img.shields.io/badge/open.mp-supported-brightgreen)

面向 **open.mp** 和 **SA-MP** 的客户端/服务端 CEF 插件。

`omp-cef` 将 Chromium Embedded Framework(CEF)嵌入到 GTA San Andreas Multiplayer 客户端中,允许服务器以多种渲染模式创建基于浏览器的 UI:Overlay2D、World2D 和 WorldObject3D。

## 特性

- GTA SA 内的离屏(Off-screen)CEF 渲染
- 用于 HUD 和菜单的 Overlay2D 浏览器渲染
- 在世界坐标位置渲染的 World2D 浏览器
- 通过对象贴图替换进行渲染的 WorldObject3D 浏览器
- 通过 `http://cef/...` 提供的打包服务端资源
- Pawn 原生函数(natives)与回调(callbacks)
- JavaScript 与 Pawn/C# 的事件桥接
- 焦点、光标、输入和音频控制
- 以 RGBA 帧形式向 CEF 界面公开的游戏画面捕获
- 自定义 ESC 菜单支持
- 自定义 TAB 菜单(计分板)支持
- 安全的客户端/服务端握手与 UDP 传输

## 文档

完整文档可在 GitHub Wiki 中查看:

https://github.com/dockfries/omp-cef/wiki

## 支持的客户端

- SA-MP 0.3.7-R1
- SA-MP 0.3.7-R3-1
- SA-MP 0.3.7-R5-1
- SA-MP 0.3.DL-R1

## 当前 CEF 版本

```txt
148.0.10+g7ee53f5+chromium-148.0.7778.218
```

## 仓库结构

```txt
src/
  client/          # 客户端插件(CEF 离屏渲染 + DX9 + SA:MP 钩子)
  server/
    common/        # 网络核心、会话、资源、协议
    omp/           # open.mp 组件(桥接 + 生命周期 + 原生函数)
    samp/          # SA-MP 插件(桥接 + 生命周期 + 原生函数)
   shared          # 共享协议/类型(数据包、序列化、通用工具)
deps/              # 依赖(minhook、sdk 等)
vcpkg.json         # vcpkg 清单(使用清单模式时)
CMakeLists.txt
CMakePresets.json  # CMake 预设(VS/CMake 的配置/构建预设)
```

## 构建

构建要求和详细的搭建说明记录在 wiki 中:

https://github.com/dockfries/omp-cef/wiki

简要版本:

- Visual Studio 2022
- CMake
- vcpkg
- Windows SDK
- 客户端(GTA SA / SA-MP)需以 x86 为目标架构

## 示例

- Overlay2D HUD
- World2D 浏览器
- WorldObject3D 浏览器
- JavaScript 与 Pawn 事件
- 浏览器生命周期
- 隐藏浏览器导航
- 压力测试
- 游戏内相机预览(演示游戏模式中的 `/camera`)

## 游戏画面捕获

CEF 页面可以通过 `window.cef.screen` 接收当前 GTA 帧。帧在 omp-cef 覆盖层渲染之前捕获,从而防止在浏览器内显示预览时发生递归捕获。

```js
const canvas = document.querySelector('canvas');
const context = canvas.getContext('2d', { alpha: false });

cef.screen.start((frame) => {
  canvas.width = frame.width;
  canvas.height = frame.height;

  const pixels = new Uint8ClampedArray(frame.data);
  context.putImageData(
    new ImageData(pixels, frame.width, frame.height),
    0,
    0
  );
}, 640, 360, 15);

addEventListener('beforeunload', () => cef.screen.stop(), { once: true });
```

`cef.screen.start(callback, width, height, fps)` 返回包含 `data`(`ArrayBuffer`,RGBA)、`width`、`height`、`sequence`、`timestamp` 和 `format` 的帧。宽度和高度被限制在 `16..1280` 和 `16..720` 之间;FPS 被限制在 `1..30`。默认值为 `640x360`、`15 FPS`。高分辨率可能会降低实际 FPS,当页面或浏览器关闭时,捕获会自动停止。

## 贡献

欢迎提交 Pull Request <3。

## 许可证

请参阅仓库中的许可证。
