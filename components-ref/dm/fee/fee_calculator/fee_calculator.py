#!/usr/bin/env python3
"""
FEE (Flash EEPROM Emulation) 配置计算工具 v2.2

更新日志:
v2.2 (2026-03-15)
    - 支持多实例FEE配置生成
    - 增加RTOS/裸机宏控制 (XY_OS_BACKEND_*)
    - 更新为新架构的 fee_handle_t (含实例级 work_buffer/flash_ops)
    - 增加调用约定速查表
    - 生成代码符合 xy_fee.h v2.0.0

v2.1 (2024-01-31)
    - 强制FEE颗粒度 >= 8字节验证
    - 区分Flash原生颗粒度和FEE颗粒度
"""

import json
import os
import sys
import tkinter as tk
from datetime import datetime
from tkinter import filedialog, messagebox, ttk

# 常量定义
VERSION = "v2.2"
FEE_MIN_GRANULARITY = 8
FEE_MAX_GRANULARITY = 128
DEFAULT_DB_PATH = os.path.join(os.path.dirname(__file__), "mcu_database.json")


class MCUDatabase:
    """MCU配置数据库管理"""

    def __init__(self, db_path=DEFAULT_DB_PATH):
        self.db_path = db_path
        self.data = None
        self.load()

    def load(self):
        """加载数据库"""
        try:
            if os.path.exists(self.db_path):
                with open(self.db_path, "r", encoding="utf-8") as f:
                    self.data = json.load(f)
            else:
                self.create_default_db()
        except Exception as e:
            messagebox.showerror("错误", f"加载数据库失败: {e}")
            self.create_default_db()

    def validate_database(self):
        """验证数据库完整性"""
        if not self.data:
            return False

        if "mcus" not in self.data:
            return False

        for mcu in self.data["mcus"]:
            # 验证颗粒度 >= 8
            if mcu.get("fee_write_granularity", 0) < FEE_MIN_GRANULARITY:
                print(
                    f"警告: {mcu['name']} 的 fee_write_granularity ({mcu['fee_write_granularity']}) 小于 {FEE_MIN_GRANULARITY}"
                )

        return True

    def save(self):
        """保存数据库"""
        try:
            with open(self.db_path, "w", encoding="utf-8") as f:
                json.dump(self.data, f, indent=2, ensure_ascii=False)
            return True
        except Exception as e:
            messagebox.showerror("错误", f"保存数据库失败: {e}")
            return False

    def create_default_db(self):
        """创建默认数据库"""
        self.data = {
            "version": "2.2",
            "last_updated": datetime.now().strftime("%Y-%m-%d"),
            "description": "FEE MCU配置数据库 - write_granularity >= 8, v2.2多实例支持",
            "mcus": [
                {
                    "id": "stm32f103",
                    "name": "STM32F103",
                    "manufacturer": "STMicroelectronics",
                    "series": "STM32F1",
                    "flash_page_size": 1024,
                    "flash_write_granularity": 4,
                    "fee_write_granularity": 8,
                    "max_erase_cycles": 10000,
                    "recommended_pages_per_fee": 2,
                },
                {
                    "id": "stm32l476",
                    "name": "STM32L476",
                    "manufacturer": "STMicroelectronics",
                    "series": "STM32L4",
                    "flash_page_size": 2048,
                    "flash_write_granularity": 8,
                    "fee_write_granularity": 8,
                    "max_erase_cycles": 10000,
                    "recommended_pages_per_fee": 2,
                },
                {
                    "id": "stm32h743",
                    "name": "STM32H743",
                    "manufacturer": "STMicroelectronics",
                    "series": "STM32H7",
                    "flash_page_size": 131072,
                    "flash_write_granularity": 32,
                    "fee_write_granularity": 32,
                    "max_erase_cycles": 100000,
                    "recommended_pages_per_fee": 1,
                },
                {
                    "id": "esp32",
                    "name": "ESP32",
                    "manufacturer": "Espressif",
                    "series": "ESP32",
                    "flash_page_size": 4096,
                    "flash_write_granularity": 4,
                    "fee_write_granularity": 8,
                    "max_erase_cycles": 100000,
                    "recommended_pages_per_fee": 2,
                },
                {
                    "id": "nrf52840",
                    "name": "nRF52840",
                    "manufacturer": "Nordic",
                    "series": "nRF52",
                    "flash_page_size": 4096,
                    "flash_write_granularity": 4,
                    "fee_write_granularity": 8,
                    "max_erase_cycles": 10000,
                    "recommended_pages_per_fee": 2,
                },
                {
                    "id": "rp2040",
                    "name": "RP2040",
                    "manufacturer": "Raspberry Pi",
                    "series": "RP2040",
                    "flash_page_size": 4096,
                    "flash_write_granularity": 256,
                    "fee_write_granularity": 256,
                    "max_erase_cycles": 100000,
                    "recommended_pages_per_fee": 2,
                },
            ],
            "manufacturers": [
                "STMicroelectronics",
                "Espressif",
                "Nordic",
                "Raspberry Pi",
            ],
        }
        self.save()


class MCUEditorDialog:
    """MCU配置编辑器对话框"""

    def __init__(self, parent, mcu=None, is_new=False):
        self.result = None
        self.is_new = is_new
        self.dialog = tk.Toplevel(parent)
        self.dialog.title("添加MCU" if is_new else "编辑MCU")
        self.dialog.geometry("500x650")
        self.dialog.transient(parent)
        self.dialog.grab_set()

        self.fields = {}
        self.create_widgets(mcu)

        # 居中显示
        self.dialog.update_idletasks()
        x = (self.dialog.winfo_screenwidth() - 500) // 2
        y = (self.dialog.winfo_screenheight() - 650) // 2
        self.dialog.geometry(f"500x650+{x}+{y}")

    def create_widgets(self, mcu):
        """创建控件"""
        frame = ttk.Frame(self.dialog, padding=10)
        frame.pack(fill=tk.BOTH, expand=True)

        # 字段定义
        fields_def = [
            ("id", "ID (唯一标识)", 50),
            ("name", "名称", 50),
            ("manufacturer", "厂商", 50),
            ("series", "系列", 50),
            ("flash_page_size", "Flash页大小(字节)", 1024),
            ("flash_write_granularity", "Flash原生写入(字节)", 4),
            ("fee_write_granularity", "FEE写入颗粒(>=8)", 8),
            ("max_erase_cycles", "最大擦除次数", 10000),
            ("recommended_pages_per_fee", "推荐Pages/FEE", 2),
        ]

        for i, (key, label, default) in enumerate(fields_def):
            ttk.Label(frame, text=label).grid(row=i, column=0, sticky=tk.W, pady=5)
            var = tk.StringVar(
                value=str(mcu.get(key, default)) if mcu else str(default)
            )
            entry = ttk.Entry(frame, textvariable=var, width=40)
            entry.grid(row=i, column=1, pady=5, padx=5)
            self.fields[key] = var

        # 按钮
        btn_frame = ttk.Frame(frame)
        btn_frame.grid(row=len(fields_def), column=0, columnspan=2, pady=20)

        ttk.Button(btn_frame, text="保存", command=self.save).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="取消", command=self.cancel).pack(
            side=tk.LEFT, padx=5
        )

    def save(self):
        """保存配置"""
        data = {key: var.get() for key, var in self.fields.items()}

        # 类型转换
        int_fields = [
            "flash_page_size",
            "flash_write_granularity",
            "fee_write_granularity",
            "max_erase_cycles",
            "recommended_pages_per_fee",
        ]
        for key in int_fields:
            try:
                data[key] = int(data[key])
            except ValueError:
                messagebox.showerror("错误", f"{key} 必须是整数")
                return

        # 验证颗粒度
        if data["fee_write_granularity"] < FEE_MIN_GRANULARITY:
            messagebox.showerror("错误", f"FEE颗粒度必须 >= {FEE_MIN_GRANULARITY}字节")
            return

        self.result = data
        self.dialog.destroy()

    def cancel(self):
        """取消"""
        self.result = None
        self.dialog.destroy()


class FEECalculator:
    """FEE计算器主窗口"""

    def __init__(self, root):
        self.root = root
        self.root.title(f"FEE配置计算工具 {VERSION}")
        self.root.geometry("900x800")

        self.db = MCUDatabase()
        self.current_config = {}

        self.create_menu()
        self.create_widgets()
        self.update_mcu_list()

        # 居中显示
        self.root.update_idletasks()
        x = (self.root.winfo_screenwidth() - 900) // 2
        y = (self.root.winfo_screenheight() - 800) // 2
        self.root.geometry(f"900x800+{x}+{y}")

    def create_menu(self):
        """创建菜单栏"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)

        # 文件菜单
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="文件", menu=file_menu)
        file_menu.add_command(label="导出配置", command=self.export_config)
        file_menu.add_command(label="导入配置", command=self.import_config)
        file_menu.add_separator()
        file_menu.add_command(label="退出", command=self.root.quit)

        # MCU数据库菜单
        db_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="MCU数据库", menu=db_menu)
        db_menu.add_command(label="管理MCU配置", command=self.manage_mcu_database)
        db_menu.add_command(label="导出数据库", command=self.export_database)
        db_menu.add_command(label="导入数据库", command=self.import_database)
        db_menu.add_separator()
        db_menu.add_command(label="刷新", command=self.refresh_database)

        # 帮助菜单
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="帮助", menu=help_menu)
        help_menu.add_command(
            label="设计约束说明", command=self.show_design_constraints
        )
        help_menu.add_command(
            label="Flash颗粒度说明", command=self.show_flash_granularity_info
        )
        help_menu.add_command(label="关于", command=self.show_about)

    def create_widgets(self):
        """创建控件"""
        # 左侧：输入面板
        left_frame = ttk.Frame(self.root, padding=10)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # MCU选择
        ttk.Label(left_frame, text="选择MCU:", font=("Arial", 10, "bold")).pack(
            anchor=tk.W
        )
        self.mcu_var = tk.StringVar()
        self.mcu_combo = ttk.Combobox(
            left_frame, textvariable=self.mcu_var, state="readonly", width=30
        )
        self.mcu_combo.pack(fill=tk.X, pady=5)
        self.mcu_combo.bind("<<ComboboxSelected>>", self.on_mcu_selected)

        ttk.Button(left_frame, text="ℹ MCU信息", command=self.show_mcu_info).pack(
            fill=tk.X
        )

        ttk.Separator(left_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=15)

        # 参数输入
        ttk.Label(left_frame, text="参数配置:", font=("Arial", 10, "bold")).pack(
            anchor=tk.W
        )

        params_frame = ttk.Frame(left_frame)
        params_frame.pack(fill=tk.X)

        # Flash页大小
        ttk.Label(params_frame, text="Flash Page Size(字节):").grid(
            row=0, column=0, sticky=tk.W, pady=5
        )
        self.flash_page_size = tk.IntVar(value=2048)
        ttk.Spinbox(
            params_frame,
            from_=256,
            to=131072,
            textvariable=self.flash_page_size,
            width=15,
        ).grid(row=0, column=1, sticky=tk.W, pady=5)

        # FEE颗粒度
        ttk.Label(params_frame, text="FEE Write Granularity(字节):").grid(
            row=1, column=0, sticky=tk.W, pady=5
        )
        self.granularity = tk.IntVar(value=8)
        gran_combo = ttk.Combobox(params_frame, textvariable=self.granularity, width=15)
        gran_combo["values"] = [8, 16, 32, 64, 128, 256]
        gran_combo.grid(row=1, column=1, sticky=tk.W, pady=5)

        # Cache大小
        ttk.Label(params_frame, text="虚拟EEPROM大小(字节):").grid(
            row=2, column=0, sticky=tk.W, pady=5
        )
        self.cache_size = tk.IntVar(value=512)
        ttk.Spinbox(
            params_frame, from_=64, to=65536, textvariable=self.cache_size, width=15
        ).grid(row=2, column=1, sticky=tk.W, pady=5)

        # Pages per FEE
        ttk.Label(params_frame, text="每个FEE Page的Flash页数:").grid(
            row=3, column=0, sticky=tk.W, pady=5
        )
        self.pages_per_fee = tk.IntVar(value=2)
        ttk.Spinbox(
            params_frame, from_=1, to=128, textvariable=self.pages_per_fee, width=15
        ).grid(row=3, column=1, sticky=tk.W, pady=5)

        # 最大擦除次数
        ttk.Label(params_frame, text="最大擦除次数:").grid(
            row=4, column=0, sticky=tk.W, pady=5
        )
        self.max_erase = tk.IntVar(value=10000)
        ttk.Spinbox(
            params_frame, from_=1000, to=1000000, textvariable=self.max_erase, width=15
        ).grid(row=4, column=1, sticky=tk.W, pady=5)

        # 日写入次数（用于寿命估算）
        ttk.Label(params_frame, text="预计每日写入次数:").grid(
            row=5, column=0, sticky=tk.W, pady=5
        )
        self.daily_writes = tk.IntVar(value=100)
        ttk.Spinbox(
            params_frame, from_=1, to=100000, textvariable=self.daily_writes, width=15
        ).grid(row=5, column=1, sticky=tk.W, pady=5)

        ttk.Separator(left_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=15)

        # 计算按钮
        ttk.Button(
            left_frame, text="计算配置", command=self.calculate, style="Accent.TButton"
        ).pack(fill=tk.X, pady=10)

        # 右侧：输出面板
        right_frame = ttk.Frame(self.root, padding=10)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        ttk.Label(right_frame, text="计算结果:", font=("Arial", 10, "bold")).pack(
            anchor=tk.W
        )

        # 输出文本框
        self.output_text = tk.Text(
            right_frame, wrap=tk.WORD, width=60, height=30, font=("Courier New", 9)
        )
        scrollbar = ttk.Scrollbar(self.output_text, command=self.output_text.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.output_text.config(yscrollcommand=scrollbar.set)
        self.output_text.pack(fill=tk.BOTH, expand=True, pady=5)

        # 按钮栏
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X)
        ttk.Button(btn_frame, text="复制代码", command=self.copy_to_clipboard).pack(
            side=tk.LEFT, padx=5
        )
        ttk.Button(btn_frame, text="保存代码", command=self.save_code).pack(
            side=tk.LEFT, padx=5
        )

    def update_mcu_list(self):
        """更新MCU列表"""
        mcus = self.db.get_all_mcus()
        names = [mcu["name"] for mcu in mcus]
        self.mcu_combo["values"] = names
        if names:
            self.mcu_var.set(names[0])

    def on_mcu_selected(self, event):
        """MCU选择事件"""
        name = self.mcu_var.get()
        mcu = self.db.get_mcu_by_name(name)
        if mcu:
            self.flash_page_size.set(mcu.get("flash_page_size", 2048))
            self.granularity.set(mcu.get("fee_write_granularity", 8))
            self.max_erase.set(mcu.get("max_erase_cycles", 10000))
            self.pages_per_fee.set(mcu.get("recommended_pages_per_fee", 2))

    def show_mcu_info(self):
        """显示MCU详细信息"""
        name = self.mcu_var.get()
        mcu = self.db.get_mcu_by_name(name)
        if not mcu:
            return

        info = f"""
MCU: {mcu["name"]}
厂商: {mcu.get("manufacturer", "N/A")}
系列: {mcu.get("series", "N/A")}

Flash Page Size: {mcu.get("flash_page_size", "N/A")} 字节
Flash原生写入: {mcu.get("flash_write_granularity", "N/A")} 字节
FEE使用颗粒: {mcu.get("fee_write_granularity", "N/A")} 字节
最大擦除次数: {mcu.get("max_erase_cycles", "N/A")}
推荐Pages/FEE: {mcu.get("recommended_pages_per_fee", "N/A")}

说明: {mcu.get("notes", "N/A")}
        """
        messagebox.showinfo("MCU信息", info.strip())

    def manage_mcu_database(self):
        """管理MCU数据库"""
        win = tk.Toplevel(self.root)
        win.title("MCU数据库管理")
        win.geometry("700x500")

        frame = ttk.Frame(win, padding=10)
        frame.pack(fill=tk.BOTH, expand=True)

        # MCU列表
        list_frame = ttk.Frame(frame)
        list_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        ttk.Label(list_frame, text="已配置的MCU:").pack(anchor=tk.W)
        listbox = tk.Listbox(list_frame, width=40)
        listbox.pack(fill=tk.BOTH, expand=True, pady=5)

        mcus = self.db.get_all_mcus()
        for mcu in mcus:
            listbox.insert(tk.END, f"{mcu['name']} ({mcu['id']})")

        # 按钮
        btn_frame = ttk.Frame(list_frame)
        btn_frame.pack()

        def add_mcu():
            dialog = MCUEditorDialog(win, is_new=True)
            win.wait_window()
            if dialog.result:
                self.db.add_mcu(dialog.result)
                self.db.save()
                self.refresh_database()

        def edit_mcu():
            sel = listbox.curselection()
            if not sel:
                return
            idx = sel[0]
            mcu = mcus[idx]
            dialog = MCUEditorDialog(win, mcu=mcu)
            win.wait_window()
            if dialog.result:
                self.db.update_mcu(mcu["id"], dialog.result)
                self.db.save()
                self.refresh_database()

        def delete_mcu():
            sel = listbox.curselection()
            if not sel:
                return
            idx = sel[0]
            mcu = mcus[idx]
            if messagebox.askyesno("确认", f"删除 {mcu['name']}?"):
                self.db.delete_mcu(mcu["id"])
                self.db.save()
                self.refresh_database()

        ttk.Button(btn_frame, text="添加", command=add_mcu).pack(fill=tk.X, pady=2)
        ttk.Button(btn_frame, text="编辑", command=edit_mcu).pack(fill=tk.X, pady=2)
        ttk.Button(btn_frame, text="删除", command=delete_mcu).pack(fill=tk.X, pady=2)

    def refresh_database(self):
        """刷新数据库"""
        self.db.load()
        self.update_mcu_list()

    def validate_database(self):
        """验证数据库"""
        if self.db.validate_database():
            messagebox.showinfo("验证", "数据库验证通过")
        else:
            messagebox.showwarning("验证", "数据库验证失败")

    def show_design_constraints(self):
        """显示设计约束说明"""
        info = """
FEE 设计约束说明

1. 最小颗粒度 = 8字节
   Record = Header(4B) + Data(gran-4B)
   如果 gran < 8，则 Data < 4B，数据空间不足

2. 每条Record = 1个write_granularity
   写入原子性以颗粒为单位

3. 2-Page架构
   - 一个ACTIVE页，一个ERASED页
   - GC时切换活动页

4. Cache是虚拟EEPROM的完整镜像
   - 掉电恢复依赖Cache
   - 读取操作直接访问Cache

5. 多实例支持 (v2.0+)
   - 每个fee_handle_t独立
   - work_buffer/flash_ops封装到handle中

6. RTOS/裸机适配 (v2.0+)
   - XY_OS_BACKEND_* 宏
   - FEE_ENTER_CRITICAL() 临界区保护
"""
        messagebox.showinfo("设计约束", info.strip())

    def show_flash_granularity_info(self):
        """显示Flash颗粒度说明"""
        info = """
Flash原生颗粒度 vs FEE颗粒度

MCU        | Flash原生 | FEE使用 | 实现方式
-----------|-----------|---------|----------
STM32F103  | 4字节     | 8字节   | 2次半字写入
STM32L476  | 8字节     | 8字节   | 1次双字写入
STM32H743  | 32字节    | 32字节  | 1次行写入
ESP32      | 4字节     | 8字节   | 软件层对齐

FEE层必须使用8字节对齐，以保证Record结构完整。
底层Flash操作可以分多次写入实现。
"""
        messagebox.showinfo("Flash颗粒度说明", info.strip())

    def show_help(self):
        """显示帮助"""
        info = """
FEE 配置计算工具使用说明

1. 选择或添加MCU型号
2. 调整参数（颗粒度必须 >= 8）
3. 点击"计算配置"
4. 查看计算结果和设计验证
5. 点击"复制代码"或"保存代码"

注意：生成的代码需要实现Flash操作接口
"""
        messagebox.showinfo("帮助", info.strip())

    def show_about(self):
        """关于"""
        messagebox.showinfo(
            "关于",
            f"FEE 配置计算工具 {VERSION}\n\n"
            "Flash EEPROM Emulation 配置生成工具\n"
            "支持多实例和RTOS/裸机适配\n\n"
            "更新: 2026-03-15",
        )

    def export_database(self):
        """导出数据库"""
        path = filedialog.asksaveasfilename(
            defaultextension=".json", filetypes=[("JSON文件", "*.json")]
        )
        if path:
            try:
                with open(path, "w", encoding="utf-8") as f:
                    json.dump(self.db.data, f, indent=2, ensure_ascii=False)
                messagebox.showinfo("成功", f"数据库已导出到\n{path}")
            except Exception as e:
                messagebox.showerror("错误", str(e))

    def import_database(self):
        """导入数据库"""
        path = filedialog.askopenfilename(filetypes=[("JSON文件", "*.json")])
        if path:
            try:
                with open(path, "r", encoding="utf-8") as f:
                    self.db.data = json.load(f)
                self.db.save()
                self.refresh_database()
                messagebox.showinfo("成功", "数据库已导入")
            except Exception as e:
                messagebox.showerror("错误", str(e))

    def export_config(self):
        """导出配置"""
        path = filedialog.asksaveasfilename(
            defaultextension=".json", filetypes=[("JSON文件", "*.json")]
        )
        if path:
            config = {
                "mcu": self.mcu_var.get(),
                "flash_page_size": self.flash_page_size.get(),
                "granularity": self.granularity.get(),
                "cache_size": self.cache_size.get(),
                "pages_per_fee": self.pages_per_fee.get(),
                "max_erase": self.max_erase.get(),
                "daily_writes": self.daily_writes.get(),
            }
            try:
                with open(path, "w", encoding="utf-8") as f:
                    json.dump(config, f, indent=2)
                messagebox.showinfo("成功", f"配置已导出到\n{path}")
            except Exception as e:
                messagebox.showerror("错误", str(e))

    def import_config(self):
        """导入配置"""
        path = filedialog.askopenfilename(filetypes=[("JSON文件", "*.json")])
        if path:
            try:
                with open(path, "r", encoding="utf-8") as f:
                    config = json.load(f)
                self.mcu_var.set(config.get("mcu", ""))
                self.flash_page_size.set(config.get("flash_page_size", 2048))
                self.granularity.set(config.get("granularity", 8))
                self.cache_size.set(config.get("cache_size", 512))
                self.pages_per_fee.set(config.get("pages_per_fee", 2))
                self.max_erase.set(config.get("max_erase", 10000))
                self.daily_writes.set(config.get("daily_writes", 100))
                self.on_mcu_selected(None)
                messagebox.showinfo("成功", "配置已导入")
            except Exception as e:
                messagebox.showerror("错误", str(e))

    def calculate(self):
        """执行计算"""
        try:
            flash_page = self.flash_page_size.get()
            gran = self.granularity.get()
            cache = self.cache_size.get()
            pages_per = self.pages_per_fee.get()
            max_erase = self.max_erase.get()
            daily_writes = self.daily_writes.get()

            # 验证
            if gran < FEE_MIN_GRANULARITY:
                messagebox.showwarning(
                    "警告", f"颗粒度({gran}B)小于最小要求({FEE_MIN_GRANULARITY}B)"
                )
                return

            # 计算
            fee_page_size = flash_page * pages_per
            total_flash = fee_page_size * 2
            aligned_header = ((8 + gran - 1) // gran) * gran
            record_data_size = gran - 4
            data_area_size = fee_page_size - aligned_header
            max_records = data_area_size // gran
            work_size = gran * 2
            total_ram = (
                len(self.current_config) + cache + work_size
                if self.current_config
                else cache + work_size
            )

            # 效率
            efficiency = (record_data_size / gran) * 100

            # 寿命估算
            records_per_gc = cache // record_data_size
            writes_per_gc = records_per_gc
            gc_per_day = daily_writes / records_per_gc if records_per_gc > 0 else 0
            days_per_gc = 1 / gc_per_day if gc_per_day > 0 else 0
            total_gc = max_erase
            total_days = total_gc * days_per_gc if days_per_gc > 0 else 0
            years = total_days / 365

            # 存储结果
            self.current_config = {
                "flash_page_size": flash_page,
                "granularity": gran,
                "cache_size": cache,
                "pages_per_fee": pages_per,
                "max_erase": max_erase,
                "daily_writes": daily_writes,
                "fee_page_size": fee_page_size,
                "total_flash": total_flash,
                "aligned_header": aligned_header,
                "record_data_size": record_data_size,
                "data_area_size": data_area_size,
                "max_records": max_records,
                "work_size": work_size,
                "total_ram": total_ram,
                "efficiency": efficiency,
                "years": years,
            }

            # 生成报告
            report = self.generate_report()
            self.output_text.delete(1.0, tk.END)
            self.output_text.insert(tk.END, report)

        except Exception as e:
            messagebox.showerror("错误", str(e))

    def generate_report(self):
        """生成计算报告"""
        if not self.current_config:
            return '请先点击"计算配置"'

        c = self.current_config
        mcu_name = self.mcu_var.get()

        report = []
        report.append("═" * 80)
        report.append(f"FEE 配置计算报告 - {mcu_name}")
        report.append(f"生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        report.append("═" * 80)
        report.append("")

        # 设计验证
        report.append("【设计验证】")
        gran = c["granularity"]
        report.append(
            f"  {'✓' if gran >= FEE_MIN_GRANULARITY else '✗'} 写入颗粒度: {gran}字节 {'(>= 8字节, 符合要求)' if gran >= FEE_MIN_GRANULARITY else '(< 8字节, 不符合要求)'}"
        )
        report.append(
            f"  {'✓' if c['record_data_size'] >= 4 else '✗'} Record数据: {c['record_data_size']}字节 (Header:4B + Data:{c['record_data_size']}B)"
        )
        report.append("")

        # FEE配置
        report.append("【FEE配置】")
        report.append(f"  FEE Page数量: 2 个")
        report.append(
            f"  单个FEE Page: {c['fee_page_size']:,} 字节 ({c['pages_per_fee']} × Flash Page)"
        )
        report.append(
            f"  总Flash占用: {c['total_flash']:,} 字节 ({c['total_flash'] / 1024:.1f} KB)"
        )
        report.append("")

        # 存储布局
        report.append("【存储布局】")
        report.append(f"  Record结构:")
        report.append(f"    ├─ 总大小: {c['granularity']} 字节 (= 1个颗粒)")
        report.append(f"    ├─ Header: 4 字节")
        report.append(f"    └─ Data: {c['record_data_size']} 字节")
        report.append(f"  对齐后Header: {c['aligned_header']} 字节")
        report.append(f"  数据区大小: {c['data_area_size']:,} 字节")
        report.append(f"  最大Record数: {c['max_records']} 条")
        report.append("")

        # RAM占用
        report.append("【RAM占用】")
        report.append(f"  Cache: {c['cache_size']} 字节")
        report.append(f"  Work Buffer: {c['work_size']} 字节")
        report.append(f"  总RAM占用: ~{c['total_ram']} 字节")
        report.append("")

        # 写入效率
        report.append("【写入效率】")
        report.append(
            f"  有效数据/Record: {c['record_data_size']}/{c['granularity']} = {c['efficiency']:.1f}%"
        )
        report.append("")

        # 性能评估
        report.append("【性能评估】")
        report.append(
            f"  预计寿命: {c['years']:.1f} 年 (每日{c['daily_writes']}次写入)"
        )
        if c["years"] < 1:
            report.append("  ⚠ 警告: 寿命不足1年，建议增加Cache或减少写入频率")
        elif c["years"] < 3:
            report.append("  ⚠ 提示: 寿命较短，高频写入场景建议优化")
        else:
            report.append("  ✓ 寿命充足")
        report.append("")

        # 多实例说明
        report.append("【多实例支持 (v2.0+)】")
        report.append("  每个实例需要独立:")
        report.append("    - virtual_eeprom[cache_size]")
        report.append("    - work_buffer[FEE_WORK_SIZE(gran)]")
        report.append("    - fee_handle_t 实例")
        report.append("")

        # RTOS/裸机说明
        report.append("【RTOS/裸机适配 (v2.0+)】")
        report.append("  - 使用 XY_OS_BACKEND_* 宏自动选择后端")
        report.append("  - 使用 FEE_ENTER_CRITICAL() 进行临界区保护")
        report.append("  - fee_read/fee_get_info 已内置保护，调用上下文灵活")
        report.append("")

        report.append("═" * 80)

        return "\n".join(report)

    def generate_c_code(self):
        """生成C配置代码"""
        try:
            flash_page = self.flash_page_size.get()
            gran = self.granularity.get()
            cache = self.cache_size.get()
            pages_per = self.pages_per_fee.get()
            max_erase = self.max_erase.get()
            mcu_name = self.mcu_var.get()

            # 验证颗粒度
            if gran < FEE_MIN_GRANULARITY:
                messagebox.showwarning(
                    "警告",
                    f"颗粒度({gran}B)小于最小要求({FEE_MIN_GRANULARITY}B)\n"
                    "生成的代码可能无法正常工作",
                )

            code = []

            # 文件头
            code.append(
                "/* ═══════════════════════════════════════════════════════════════"
            )
            code.append(" * FEE (Flash EEPROM Emulation) 配置代码")
            code.append(" * 自动生成 by FEE Calculator v2.2")
            code.append(f" * MCU: {mcu_name}")
            code.append(f" * 生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            code.append(
                " * ═══════════════════════════════════════════════════════════════*/"
            )
            code.append("")
            code.append(
                "/* ============================================================"
            )
            code.append(" * 重要更新 (v2.0+):")
            code.append(" *   - 支持多实例 (每个fee_handle_t独立)")
            code.append(" *   - 支持RTOS/裸机适配 (XY_OS_BACKEND_*)")
            code.append(" *   - work_buffer/flash_ops封装到handle中")
            code.append(
                " * ============================================================ */"
            )
            code.append("")

            # 头文件
            code.append('#include "xy_fee.h"')
            code.append("#include <string.h>")
            code.append("")

            # ========== 平台检测 ==========
            code.append(
                "/* ============================================================"
            )
            code.append(" * 平台和后端配置")
            code.append(
                " * ============================================================ */"
            )
            code.append("")
            code.append("/*")
            code.append(" * 选择后端模式:")
            code.append(" *   - XY_OS_BACKEND_BAREMETAL  (裸机，无RTOS)")
            code.append(" *   - XY_OS_BACKEND_FREERTOS   (FreeRTOS)")
            code.append(" *   - XY_OS_BACKEND_RTTHREAD   (RT-Thread)")
            code.append(" *")
            code.append(" * 默认: 裸机模式 (XY_OS_BACKEND_BAREMETAL)")
            code.append(" * 如需修改，请在编译时定义这些宏")
            code.append(" */")
            code.append("")
            code.append("#ifndef XY_OS_BACKEND_BAREMETAL")
            code.append("    #define XY_OS_BACKEND_BAREMETAL  1")
            code.append("    #define XY_OS_BACKEND_FREERTOS   0")
            code.append("    #define XY_OS_BACKEND_RTTHREAD   0")
            code.append("#endif")
            code.append("")
            code.append("/* 临界区适配宏 */")
            code.append("#if XY_OS_BACKEND_BAREMETAL")
            code.append(
                "    #define FEE_ENTER_CRITICAL()    do { } while (0)  /* 调用者保证禁中断 */"
            )
            code.append("    #define FEE_EXIT_CRITICAL()     do { } while (0)")
            code.append("#elif XY_OS_BACKEND_FREERTOS")
            code.append("    #define FEE_ENTER_CRITICAL()    taskENTER_CRITICAL()")
            code.append("    #define FEE_EXIT_CRITICAL()     taskEXIT_CRITICAL()")
            code.append("#elif XY_OS_BACKEND_RTTHREAD")
            code.append("    #define FEE_ENTER_CRITICAL()    rt_enter_critical()")
            code.append("    #define FEE_EXIT_CRITICAL()     rt_exit_critical()")
            code.append("#else")
            code.append("    #define FEE_ENTER_CRITICAL()    do { } while (0)")
            code.append("    #define FEE_EXIT_CRITICAL()     do { } while (0)")
            code.append("#endif")
            code.append("")

            # ========== 设计验证 ==========
            code.append(
                "/* ============================================================"
            )
            code.append(" * 设计验证")
            code.append(
                " * ============================================================ */"
            )
            code.append(f"#if FEE_MIN_GRANULARITY > {gran}")
            code.append(f'#error "FEE写入颗粒度必须 >= {FEE_MIN_GRANULARITY}字节"')
            code.append("#endif")
            code.append("")

            # ========== 多实例定义 ==========
            code.append(
                "/* ============================================================"
            )
            code.append(" * 多实例配置")
            code.append(
                " * ============================================================ */"
            )
            code.append("")
            code.append("/*")
            code.append(" * 示例: 为不同组件定义独立的FEE实例")
            code.append(" *")
            code.append(" * 使用方式:")
            code.append(" *   - 网络组件使用 net_fee 实例")
            code.append(" *   - 参数存储使用 param_fee 实例")
            code.append(" *   - 每个实例独立管理，互不影响")
            code.append(" */")
            code.append("")
            code.append(
                "/* ────────────────────────────────────────────────────────────── */"
            )
            code.append("/* 1. 网络组件FEE实例 */")
            code.append(
                "/* ────────────────────────────────────────────────────────────── */"
            )
            code.append(f"static uint8_t net_virtual_eeprom[{cache}];")
            code.append(
                f"static uint8_t net_work_buffer[FEE_WORK_SIZE({gran})];  /* {gran * 2} bytes */"
            )
            code.append("static fee_handle_t net_fee;")
            code.append("")
            code.append(
                "/* ────────────────────────────────────────────────────────────── */"
            )
            code.append("/* 2. 参数存储FEE实例 (可选) */")
            code.append(
                "/* ────────────────────────────────────────────────────────────── */"
            )
            code.append(f"// static uint8_t param_virtual_eeprom[256];")
            code.append(f"// static uint8_t param_work_buffer[FEE_WORK_SIZE({gran})];")
            code.append(f"// static fee_handle_t param_fee;")
            code.append("")

            # ========== Flash操作函数 ==========
            code.append(
                "/* ============================================================"
            )
            code.append(" * Flash操作接口实现")
            code.append(
                " * ============================================================ */"
            )
            code.append("")
            code.append("/* TODO: 根据具体MCU实现以下函数 */")
            code.append("")
            code.append("/**")
            code.append(" * @brief 擦除Flash页")
            code.append(" * @param addr 页起始地址")
            code.append(" * @return 0-成功，非0-失败")
            code.append(" */")
            code.append(f"static int fee_flash_erase(uint32_t addr) {{")
            code.append(f"    /* TODO: 实现Flash页擦除，页大小 = {flash_page} 字节 */")
            code.append("    /*")
            code.append("    Example for STM32:")
            code.append("    FLASH_EraseInitTypeDef erase;")
            code.append("    uint32_t page_error;")
            code.append("    HAL_FLASH_Unlock();")
            code.append("    erase.TypeErase = FLASH_TYPEERASE_PAGES;")
            code.append(f"    erase.PageAddress = addr;")
            code.append("    erase.NbPages = 1;")
            code.append("    HAL_FLASHEx_Erase(&erase, &page_error);")
            code.append("    HAL_FLASH_Lock();")
            code.append("    */")
            code.append("    return 0;  /* 0-成功, 非0-失败 */")
            code.append("}")
            code.append("")
            code.append("/**")
            code.append(" * @brief 写入Flash")
            code.append(" * @param addr 写入地址")
            code.append(" * @param data 数据指针")
            code.append(f" * @param len 长度（必须是{gran}的整数倍）")
            code.append(" * @return 0-成功，非0-失败")
            code.append(" *")
            code.append(" * @note 地址和长度必须按 write_granularity 对齐")
            code.append(" */")
            code.append(
                f"static int fee_flash_write(uint32_t addr, const uint8_t *data, uint16_t len) {{"
            )
            code.append(f"    /* TODO: 实现Flash写入，长度必须按{gran}字节对齐 */")
            code.append(f"    /*")
            code.append(f"    Example for STM32 (gran=8, 4B->8B wrapper):")
            code.append("    HAL_FLASH_Unlock();")
            code.append("    for (uint16_t i = 0; i < len; i += 4) {")
            code.append("        uint32_t word = *(uint32_t *)(data + i);")
            code.append(
                "        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word);"
            )
            code.append("    }")
            code.append("    HAL_FLASH_Lock();")
            code.append("    */")
            code.append("    return 0;  /* 0-成功, 非0-失败 */")
            code.append("}")
            code.append("")
            code.append("/**")
            code.append(" * @brief 读取Flash")
            code.append(" * @param addr 读取地址")
            code.append(" * @param data 数据缓冲区")
            code.append(" * @param len 读取长度")
            code.append(" * @return 0-成功，非0-失败")
            code.append(" */")
            code.append(
                "static int fee_flash_read(uint32_t addr, uint8_t *data, uint16_t len) {"
            )
            code.append("    /* TODO: 实现Flash读取 */")
            code.append("    memcpy(data, (void *)addr, len);")
            code.append("    return 0;  /* 0-成功, 非0-失败 */")
            code.append("}")
            code.append("")

            # Flash操作接口
            code.append("/* Flash操作接口 */")
            code.append("static const fee_flash_ops_t fee_flash_ops = {")
            code.append("    .erase = fee_flash_erase,")
            code.append("    .write = fee_flash_write,")
            code.append("    .read  = fee_flash_read")
            code.append("};")
            code.append("")

            # ========== FEE配置参数 ==========
            code.append(
                "/* ============================================================"
            )
            code.append(" * FEE配置参数")
            code.append(
                " * ============================================================ */"
            )
            code.append("")
            code.append("/* 网络组件FEE配置 */")
            code.append("static const fee_config_t net_fee_config = {")
            code.append(
                f"    .flash_base         = (uint8_t *)0x08010000,  /* TODO: 修改为实际地址 */"
            )
            code.append(
                f"    .pages_per_fee_page = {pages_per},            /* 每个FEE Page = {pages_per} × Flash Page */"
            )
            code.append(
                f"    .flash_page_size    = {flash_page},          /* Flash Page大小 */"
            )
            code.append(
                f"    .cache_size         = {cache},               /* 虚拟EEPROM大小 */"
            )
            code.append(
                f"    .write_granularity  = {gran},                /* 写入颗粒度 (>= {FEE_MIN_GRANULARITY}) */"
            )
            code.append(
                f"    .max_erase_count    = {max_erase},           /* 最大擦除次数 */"
            )
            code.append(f"    .flash_ops          = &fee_flash_ops")
            code.append("};")
            code.append("")

            # ========== 初始化函数 ==========
            code.append(
                "/* ============================================================"
            )
            code.append(" * 初始化函数")
            code.append(
                " * ============================================================ */"
            )
            code.append("")
            code.append("/**")
            code.append(" * @brief 初始化FEE系统")
            code.append(" *")
            code.append(" * @note 调用上下文:")
            code.append(" *       - 裸机: 禁中断状态")
            code.append(" *       - RTOS: 持有互斥锁")
            code.append(" */")
            code.append("int fee_system_init(void) {")
            code.append("    fee_status_t status;")
            code.append("")
            code.append("    /* 初始化网络组件FEE */")
            code.append("    status = fee_init(&net_fee, &net_fee_config,")
            code.append(
                "                         net_virtual_eeprom, net_work_buffer);"
            )
            code.append("    if (status != FEE_OK) {")
            code.append("        /* 初始化失败处理 */")
            code.append("        return -1;")
            code.append("    }")
            code.append("")
            code.append("    /* 可选: 初始化其他FEE实例 */")
            code.append("    // status = fee_init(&param_fee, &param_fee_config,")
            code.append(
                "    //                      param_virtual_eeprom, param_work_buffer);"
            )
            code.append("")
            code.append("    return 0;")
            code.append("}")
            code.append("")

            # ========== 使用示例 ==========
            code.append(
                "/* ============================================================"
            )
            code.append(" * 使用示例")
            code.append(
                " * ============================================================ */"
            )
            code.append("")
            code.append("/**")
            code.append(" * @brief 写入配置数据")
            code.append(" */")
            code.append("int save_config(void) {")
            code.append("    struct config {")
            code.append("        uint32_t baudrate;")
            code.append("        uint16_t timeout;")
            code.append("        uint8_t mode;")
            code.append(
                "    } cfg = { .baudrate = 115200, .timeout = 1000, .mode = 1 };"
            )
            code.append("")
            code.append("    /*")
            code.append("     * 裸机调用方式:")
            code.append("     *   xy_enter_critical();")
            code.append(
                "     *   fee_write(&net_fee, 0, (uint8_t *)&cfg, sizeof(cfg));"
            )
            code.append("     *   xy_exit_critical();")
            code.append("     *")
            code.append("     * RTOS调用方式:")
            code.append("     *   xy_os_mutex_lock(&fee_mutex);")
            code.append(
                "     *   fee_write(&net_fee, 0, (uint8_t *)&cfg, sizeof(cfg));"
            )
            code.append("     *   xy_os_mutex_unlock(&fee_mutex);")
            code.append("     */")
            code.append(
                "    return fee_write(&net_fee, 0, (uint8_t *)&cfg, sizeof(cfg)) == FEE_OK ? 0 : -1;"
            )
            code.append("}")
            code.append("")
            code.append("/**")
            code.append(" * @brief 读取配置数据")
            code.append(" *")
            code.append(" * @note fee_read 已内置临界区保护，调用上下文灵活")
            code.append(" */")
            code.append("int load_config(struct config *cfg) {")
            code.append("    if (cfg == NULL) return -1;")
            code.append(
                "    return fee_read(&net_fee, 0, (uint8_t *)cfg, sizeof(*cfg)) == FEE_OK ? 0 : -1;"
            )
            code.append("}")
            code.append("")

            # ========== 调用约定速查表 ==========
            code.append(
                "/* ============================================================"
            )
            code.append(" * 调用约定速查表")
            code.append(
                " * ============================================================ */"
            )
            code.append("/*")
            code.append(
                " * +-------------------+---------------------------+---------------------------+"
            )
            code.append(
                " * | 函数              | 裸机（禁中断）            | RTOS（持有锁）            |"
            )
            code.append(
                " * +-------------------+---------------------------+---------------------------+"
            )
            code.append(
                " * | fee_init          | 禁中断                   | 持有互斥锁                |"
            )
            code.append(
                " * | fee_write         | 禁中断                   | 持有互斥锁                |"
            )
            code.append(
                " * | fee_read          | 任意（已保护）           | 任意（已保护）            |"
            )
            code.append(
                " * | fee_format        | 禁中断                   | 持有互斥锁                |"
            )
            code.append(
                " * | fee_gc            | 禁中断                   | 持有互斥锁                |"
            )
            code.append(
                " * | fee_get_info      | 任意（已保护）           | 任意（已保护）            |"
            )
            code.append(
                " * +-------------------+---------------------------+---------------------------+"
            )
            code.append(" */")
            code.append("")

            return "\n".join(code)

        except Exception as e:
            return f"代码生成失败: {str(e)}"

    def copy_to_clipboard(self):
        """复制代码到剪贴板"""
        if not self.current_config:
            messagebox.showwarning("提示", '请先点击"计算配置"')
            return

        code = self.generate_c_code()
        self.root.clipboard_clear()
        self.root.clipboard_write(code)
        messagebox.showinfo("成功", "代码已复制到剪贴板")

    def save_code(self):
        """保存代码到文件"""
        if not self.current_config:
            messagebox.showwarning("提示", '请先点击"计算配置"')
            return

        path = filedialog.asksaveasfilename(
            defaultextension=".c", filetypes=[("C文件", "*.c"), ("所有文件", "*.*")]
        )
        if path:
            try:
                code = self.generate_c_code()
                with open(path, "w", encoding="utf-8") as f:
                    f.write(code)
                messagebox.showinfo("成功", f"代码已保存到\n{path}")
            except Exception as e:
                messagebox.showerror("错误", str(e))


def main():
    """主函数"""
    root = tk.Tk()
    app = FEECalculator(root)
    root.mainloop()


if __name__ == "__main__":
    main()
