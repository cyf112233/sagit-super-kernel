# cxkcxkckx BakaSU Kernel — sagit / msm8998

基于 **Linux 4.4.302-cip115**（小米 6 / msm8998）重新编译的内核，集成 **BakaSU + SUSFS + NoMount(Prism) + BBR**。

- 内核版本串：`4.4.302-cxkcxkckx-bakasu-susfs-nomount-prism-bbr-cip115`
- 设备代号：`sagit`（msm8998，A-only，boot 分区 `/dev/block/bootdevice/by-name/boot`）
- 原始内核来源：酷安 [@4339418](https://www.coolapk.com/u/4339418)

---

## 功能

| 模块 | 状态 | 说明 |
|---|---|---|
| BakaSU | ✅ | `v4.2.0-rc3`，KSU version code **35252**，`KERNEL_SU_UAPI_VERSION 6`；支持官方 KernelSU / KOWSU / BakaSU 管理器签名 |
| SUSFS | ✅ | `sus_path` `sus_mount` `sus_kstat` `spoof_uname` `open_redirect` `sus_map` `hide_ksu_susfs_symbols` `spoof_cmdline_or_bootconfig` |
| NoMount | ✅ | **Prism 引擎 v34**（genl/netlink proto 29），已移植到 4.4；配套 NoMount Suite `00_NoMount-Module` 使用 |
| TCP | ✅ | 默认拥塞控制 **BBR**，启用 `NET_SCH_FQ`；`westwood`/`cubic` 仍可切换 |
| 调度 | ✅ | `HZ=300`、`PREEMPT`、`SLUB`、`CC_OPTIMIZE_FOR_PERFORMANCE`、`CPU_BOOST` |
| I/O | ✅ | 默认 `cfq`，同时编入 `bfq`/`deadline`/`noop` |
| ZRAM | ✅ | `lz4kd` 默认算法 |

> 说明：本内核**不含**内核级 Zygisk（如 YukiZygisk），此类实现要求 GKI/KMI（5.10+），4.4 无法加载。请使用用户态 Zygisk 方案（Zygisk Next / zygisksu）。

---

## 目录结构

```
kernel/                内核源码（4.4.302-cip115 + 上述集成）
out-bakasu/            构建产物
  ├── Image            未压缩内核
  ├── Image.gz         gzip 内核
  ├── Image.gz-dtb     追加了 DTB 的完整镜像（刷机用）
  ├── vmlinux
  ├── System.map
  └── .config          实际使用的配置
ak3/                   AnyKernel3 打包目录（已放好 Image.gz-dtb 与 anykernel.sh）
manager/               配套模块与管理器 APK
```

---

## 自行编译

### 依赖
- `clang 13` + `lld 13`（本项目用 Debian clang 13.0.1；原作者使用 Proton clang 13，两者均可）
- `bc bison flex libssl-dev libelf-dev cpio device-tree-compiler`

### 命令
```bash
export PATH=/opt/clang13/bin:$PATH        # 指向 clang-13/ld.lld-13/llvm-* 的目录
make O=out-build ARCH=arm64 LLVM=1 olddefconfig
make O=out-build ARCH=arm64 LLVM=1 -j$(nproc) Image.gz-dtb
```

**必须使用 `O=` 外部构建**：这棵树只在 out-of-tree 模式下给每个目录追加 `-I$(srctree)/$(src)`（见 `scripts/Makefile.lib`），in-tree 构建会因找不到本地头文件而失败。

### 产物与 DTB
源码包中 `arch/arm64/boot/dts/qcom/` 为空（不含 QCOM DTS），因此 `Image.gz-dtb` 只会等于 `Image.gz`。
需要刷机时请自行把设备对应的 FDT 追加到 `Image.gz` 末尾：

```bash
cat Image.gz device.dtb > Image.gz-dtb     # device.dtb 为从原厂/可用 boot.img 提取的 DTB
```

本项目产物中 `Image.gz-dtb = Image.gz(14414285) + DTB(408314)`，DTB 取自原编译产物，FDT `totalsize` 与长度一致。

---

## 刷入

1. 刷 AK3 包（TWRP / Kernel Flasher），设备校验 `sagit`；
2. 重启后安装 NoMount Suite 模块 `00_NoMount-Module-*.zip`，再重启；
3. 验证：
   ```sh
   uname -r                       # 4.4.302-cxkcxkckx-...
   nomount vfs list | head        # 规则列表
   cat /sys/module/*/parameters/* # 按需
   ```

---

## 4.4 适配记录（相对原始源码的改动）

这棵树从 Windows 环境拷贝而来，且上游多处按 4.7+/5.x 编写，移植到 4.4 时做了以下修复：

1. **换行与权限**：全树 CRLF → LF；恢复 `scripts/`、`*.sh` 可执行位。
2. **大小写撞名**（Windows 只保留一份）：补回 `xt_mark.h`、`xt_connmark.h`（含 shift 字段）、`xt_CONNMARK.h`、`ipt_ttl.h`/`ipt_TTL.h`、`xt_tcpmss.h` 的 match 结构；软链 `xt_dscp.h`、`ip6t_hl.h`、`ipt_ECN.h`、`net/netfilter/xt_{dscp,hl,tcpmss}.c`。
3. **蓝牙**：`drivers/bluetooth/Makefile` 补 `-I$(srctree)/drivers/bluetooth`（`btfm_slim` 用尖括号包含本地头）。
4. **BBR 回移补全**：`struct sock` 增加 `sk_pacing_shift`（默认 10），`net/ipv4/Kconfig` 补 `default "bbr" if DEFAULT_BBR`。
5. **NoMount/Prism 移植到 4.4**：
   - `full_name_hash` 三参数 → 两参数（`nm_fnh` 宏）
   - `iterate_shared` 按 4.7 分界屏蔽，只保留 `iterate`；`nm_get_fop` 增加 <4.7 分支
   - `get_link`/`delayed_call` → `follow_link`/`put_link`
   - **`IOP_NOFOLLOW`**：这棵树的 `d_flags_for_inode()` 会把带 `follow_link` 的 inode 判成 `DCACHE_SYMLINK_TYPE`；必须给非链接 inode 打 `IOP_NOFOLLOW`，否则每个注入文件都会被当作符号链接遍历，`open()` 直接 `-EINVAL`
   - `.read`/`.write` 转发：4.4 的 `vfs_read()` 优先走 `f_op->read`，仅有 `read_iter` 会 EINVAL
   - `xattr_prefix()` → `handler->prefix`；xattr handler get/set 改 4.4 签名
   - `lookup_one_len_unlocked` → `lookup_one_len`；`thp_get_unmapped_area` 仅 ≥4.20 挂载
   - `i_opflags | IOP_XATTR` 仅在有宏时设置
6. **BakaSU 版本固定**：`drivers/kernelsu/Kbuild` 中 `KSU_LOCAL_VERSION := 4552` → version code 35252（与上游 `v4.2.0-rc3` 对齐）。

---

## 已知问题

- **无 QCOM DTS**：源码包 `arch/arm64/boot/dts/qcom` 为空，`Image.gz-dtb` 需手动追加 DTB。
- **ghost / pathhide 不可用**：NoMount 的这两项在 built-in 模式下由预编译 LKM 提供（源码未公开），本内核只有主引擎（maps 与 xattr 由 `task_mmu.c`/`xattr.c` 的直接 hook 覆盖）。
- **`FRAME_POINTER` 无法关闭**：`arch/arm64/Kconfig` 中 `select ARCH_WANT_FRAME_POINTERS`。
- 4.4 非 NoMount 官方支持版本（官方 patch 面向 4.9+），如遇异常请先看 `dmesg | grep -i nomount`。
- BakaSU 版本码为 35252，比当前管理器（如 ReSukiSU 35171）新，管理器可能提示“管理器过旧”；如需匹配可把 `KSU_LOCAL_VERSION` 改为 `4471`（version code 35171）后重编。

---

## 致谢

- **酷安 [@4339418](https://www.coolapk.com/u/4339418)**：原始 msm8998 / CIP115 内核
- **BakaSU / ReSukiSU**（KernelSU 下游）与 **tiann/KernelSU**
- **SUSFS**（simonpunk / 5ec1cff）
- **NoMount**（maxsteeel）与 **Prism hookless 引擎**（Bouteillepleine/NoMount-Suite）
- **AnyKernel3**（osm0sis）
- **Clang/LLD 13**（Debian；原作者使用 Proton clang 13）

## 许可

内核源码遵循 **GPL-2.0**；各集成组件保留其原始许可（BakaSU kernel 目录 GPL-2.0-only，SUSFS GPL-2.0，NoMount GPL-2.0，AnyKernel3 见其仓库）。
