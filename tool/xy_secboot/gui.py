#!/usr/bin/env python3
"""Tkinter GUI for XY SecBoot host tools."""

from __future__ import annotations

import queue
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from .constants import APP_IMAGE_ADDR_N32, PRODUCT_ID_N32, SUITE_MARKET, UART_DEFAULT_PAYLOAD
from .manifest import Manifest
from .package import SecbootPackage, align_image
from .uart import SecbootUartClient, available_ports


class SecbootGui(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("XY SecBoot Host")
        self.geometry("820x620")
        self.events: queue.Queue[tuple[str, object]] = queue.Queue()
        self.worker: threading.Thread | None = None

        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value="115200")
        self.package_var = tk.StringVar()
        self.image_var = tk.StringVar()
        self.output_var = tk.StringVar()
        self.product_var = tk.StringVar(value=f"0x{PRODUCT_ID_N32:08x}")
        self.image_addr_var = tk.StringVar(value=f"0x{APP_IMAGE_ADDR_N32:08x}")
        self.entry_addr_var = tk.StringVar(value=f"0x{APP_IMAGE_ADDR_N32:08x}")
        self.version_var = tk.StringVar(value="1")
        self.counter_var = tk.StringVar(value="1")
        self.key_id_var = tk.StringVar(value="dev-key-01")
        self.payload_var = tk.StringVar(value=str(UART_DEFAULT_PAYLOAD))
        self.retries_var = tk.StringVar(value="10")
        self.timeout_var = tk.StringVar(value="1000")
        self.progress_var = tk.DoubleVar(value=0.0)

        self._build_ui()
        self.after(100, self._drain_events)

    def _build_ui(self) -> None:
        root = ttk.Frame(self, padding=12)
        root.pack(fill=tk.BOTH, expand=True)

        conn = ttk.LabelFrame(root, text="Serial", padding=8)
        conn.pack(fill=tk.X)
        ttk.Label(conn, text="Port").grid(row=0, column=0, sticky=tk.W)
        ttk.Combobox(conn, textvariable=self.port_var, values=self._safe_ports(), width=18).grid(row=0, column=1, sticky=tk.W, padx=4)
        ttk.Button(conn, text="Refresh", command=self._refresh_ports).grid(row=0, column=2, padx=4)
        ttk.Label(conn, text="Baud").grid(row=0, column=3, sticky=tk.W, padx=(20, 0))
        ttk.Entry(conn, textvariable=self.baud_var, width=10).grid(row=0, column=4, sticky=tk.W, padx=4)
        ttk.Button(conn, text="HELLO", command=self.hello).grid(row=0, column=5, padx=4)

        pack = ttk.LabelFrame(root, text="Package", padding=8)
        pack.pack(fill=tk.X, pady=8)
        self._file_row(pack, 0, "App bin", self.image_var, self._choose_image)
        self._file_row(pack, 1, "Output .sbp", self.output_var, self._choose_output)
        ttk.Label(pack, text="Product").grid(row=2, column=0, sticky=tk.W)
        ttk.Entry(pack, textvariable=self.product_var, width=14).grid(row=2, column=1, sticky=tk.W, padx=4)
        ttk.Label(pack, text="Image addr").grid(row=2, column=2, sticky=tk.W)
        ttk.Entry(pack, textvariable=self.image_addr_var, width=14).grid(row=2, column=3, sticky=tk.W, padx=4)
        ttk.Label(pack, text="Entry addr").grid(row=2, column=4, sticky=tk.W)
        ttk.Entry(pack, textvariable=self.entry_addr_var, width=14).grid(row=2, column=5, sticky=tk.W, padx=4)
        ttk.Label(pack, text="Version").grid(row=3, column=0, sticky=tk.W)
        ttk.Entry(pack, textvariable=self.version_var, width=10).grid(row=3, column=1, sticky=tk.W, padx=4)
        ttk.Label(pack, text="Counter").grid(row=3, column=2, sticky=tk.W)
        ttk.Entry(pack, textvariable=self.counter_var, width=10).grid(row=3, column=3, sticky=tk.W, padx=4)
        ttk.Label(pack, text="Key ID").grid(row=3, column=4, sticky=tk.W)
        ttk.Entry(pack, textvariable=self.key_id_var, width=14).grid(row=3, column=5, sticky=tk.W, padx=4)
        ttk.Button(pack, text="Build Package", command=self.build_package).grid(row=4, column=0, pady=8, sticky=tk.W)

        flash = ttk.LabelFrame(root, text="Flash", padding=8)
        flash.pack(fill=tk.X)
        self._file_row(flash, 0, "Package", self.package_var, self._choose_package)
        ttk.Label(flash, text="Payload").grid(row=1, column=0, sticky=tk.W)
        ttk.Entry(flash, textvariable=self.payload_var, width=10).grid(row=1, column=1, sticky=tk.W, padx=4)
        ttk.Label(flash, text="Retries").grid(row=1, column=2, sticky=tk.W)
        ttk.Entry(flash, textvariable=self.retries_var, width=10).grid(row=1, column=3, sticky=tk.W, padx=4)
        ttk.Label(flash, text="Timeout ms").grid(row=1, column=4, sticky=tk.W)
        ttk.Entry(flash, textvariable=self.timeout_var, width=10).grid(row=1, column=5, sticky=tk.W, padx=4)
        ttk.Button(flash, text="Flash Package", command=self.flash_package).grid(row=2, column=0, pady=8, sticky=tk.W)
        ttk.Progressbar(flash, variable=self.progress_var, maximum=100).grid(row=2, column=1, columnspan=5, sticky=tk.EW, padx=4)

        log_box = ttk.LabelFrame(root, text="Log", padding=8)
        log_box.pack(fill=tk.BOTH, expand=True, pady=8)
        self.log = tk.Text(log_box, height=14, wrap=tk.WORD)
        self.log.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scroll = ttk.Scrollbar(log_box, orient=tk.VERTICAL, command=self.log.yview)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.log.configure(yscrollcommand=scroll.set)

    def _file_row(self, parent: ttk.Frame, row: int, label: str, var: tk.StringVar, command) -> None:
        ttk.Label(parent, text=label).grid(row=row, column=0, sticky=tk.W)
        ttk.Entry(parent, textvariable=var, width=72).grid(row=row, column=1, columnspan=5, sticky=tk.EW, padx=4)
        ttk.Button(parent, text="Browse", command=command).grid(row=row, column=6, padx=4)

    def _safe_ports(self) -> list[str]:
        try:
            return available_ports()
        except Exception:
            return []

    def _refresh_ports(self) -> None:
        ports = self._safe_ports()
        self.log_line(f"ports: {', '.join(ports) if ports else 'none'}")

    def _choose_image(self) -> None:
        path = filedialog.askopenfilename(title="Select app binary", filetypes=[("Binary", "*.bin"), ("All", "*.*")])
        if path:
            self.image_var.set(path)
            if not self.output_var.get():
                self.output_var.set(str(Path(path).with_suffix(".sbp")))

    def _choose_output(self) -> None:
        path = filedialog.asksaveasfilename(title="Save package", defaultextension=".sbp", filetypes=[("SecBoot package", "*.sbp")])
        if path:
            self.output_var.set(path)

    def _choose_package(self) -> None:
        path = filedialog.askopenfilename(title="Select package", filetypes=[("SecBoot package", "*.sbp"), ("All", "*.*")])
        if path:
            self.package_var.set(path)

    def log_line(self, text: str) -> None:
        self.log.insert(tk.END, text + "\n")
        self.log.see(tk.END)

    def _start_worker(self, target) -> None:
        if self.worker and self.worker.is_alive():
            messagebox.showwarning("Busy", "Another operation is running")
            return
        self.worker = threading.Thread(target=target, daemon=True)
        self.worker.start()

    def build_package(self) -> None:
        def work() -> None:
            try:
                raw_image = Path(self.image_var.get()).read_bytes()
                image = align_image(raw_image)
                manifest = Manifest.build(
                    image,
                    product_id=int(self.product_var.get(), 0),
                    image_addr=int(self.image_addr_var.get(), 0),
                    entry_addr=int(self.entry_addr_var.get(), 0),
                    image_version=int(self.version_var.get(), 0),
                    security_counter=int(self.counter_var.get(), 0),
                    key_id=self.key_id_var.get().encode("utf-8"),
                )
                package = SecbootPackage.build(image, manifest, SUITE_MARKET)
                package.write(self.output_var.get())
                pad_note = "" if len(image) == len(raw_image) else f", padded from {len(raw_image)}"
                self.events.put(("log", f"built {self.output_var.get()} ({len(image)} bytes image{pad_note})"))
                self.events.put(("package", self.output_var.get()))
            except Exception as exc:
                self.events.put(("error", str(exc)))

        self._start_worker(work)

    def hello(self) -> None:
        def work() -> None:
            try:
                with SecbootUartClient(self.port_var.get(), int(self.baud_var.get(), 0), int(self.timeout_var.get(), 0) / 1000.0) as client:
                    caps = client.hello()
                self.events.put(("log", f"CAPS {caps.payload.hex()}"))
            except Exception as exc:
                self.events.put(("error", str(exc)))

        self._start_worker(work)

    def flash_package(self) -> None:
        def work() -> None:
            try:
                package = SecbootPackage.read(self.package_var.get())

                def progress(done: int, total: int) -> None:
                    percent = 100.0 if total == 0 else min(100.0, done * 100.0 / total)
                    self.events.put(("progress", percent))

                with SecbootUartClient(self.port_var.get(), int(self.baud_var.get(), 0), int(self.timeout_var.get(), 0) / 1000.0) as client:
                    caps = client.hello()
                    self.events.put(("log", f"CAPS {caps.payload.hex()}"))
                    ack = client.flash_package(package, int(self.payload_var.get(), 0), int(self.retries_var.get(), 0), progress)
                self.events.put(("log", f"flash done: {ack.describe()}"))
            except Exception as exc:
                self.events.put(("error", str(exc)))

        self.progress_var.set(0)
        self._start_worker(work)

    def _drain_events(self) -> None:
        while True:
            try:
                kind, value = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == "log":
                self.log_line(str(value))
            elif kind == "error":
                self.log_line(f"ERROR: {value}")
                messagebox.showerror("SecBoot", str(value))
            elif kind == "progress":
                self.progress_var.set(float(value))
            elif kind == "package":
                self.package_var.set(str(value))
        self.after(100, self._drain_events)


def main() -> int:
    app = SecbootGui()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
