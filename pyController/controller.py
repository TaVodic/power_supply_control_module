import threading
import tkinter as tk
from tkinter import ttk, messagebox

import serial
import serial.tools.list_ports


class PSUApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Power Monitor")
        self.root.geometry("420x560")
        self.root.configure(bg="#111111")

        self.ser = None
        self.read_thread = None
        self.running = False

        self.v_var = tk.StringVar(value="30.00")
        self.c_var = tk.StringVar(value="5.000")
        self.p_var = tk.StringVar(value="5.000")
        self.input_voltage_var = tk.StringVar()
        self.input_current_var = tk.StringVar()
        self.wiper_voltage_var = tk.StringVar(value="--")
        self.wiper_current_var = tk.StringVar(value="--")
        self.port_var = tk.StringVar()

        self.build_ui()
        self.refresh_ports()
        self.auto_connect_cp2102()

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def build_ui(self):
        num_vcmd = (self.root.register(self.validate_numeric), "%P")

        self.top = tk.Frame(self.root, bg="#111111")
        self.top.pack(fill="x", padx=14, pady=(12, 8))

        self.port_box = ttk.Combobox(
            self.top,
            textvariable=self.port_var,
            state="readonly",
            width=18
        )
        self.port_box.pack(side="left", padx=(0, 8))

        refresh_btn = ttk.Button(self.top, text="Refresh", command=self.refresh_ports)
        refresh_btn.pack(side="left", padx=(0, 8))

        self.conn_btn = ttk.Button(self.top, text="Connect", command=self.toggle_connection)
        self.conn_btn.pack(side="left")

        self.shell = tk.Frame(self.root, bg="#1b1b1b", bd=2, relief="ridge")
        self.shell.pack(fill="both", expand=True, padx=14, pady=12)

        bezel = tk.Frame(self.root, bg="#2a2a2a")
        bezel.place(in_=self.shell, relx=0.5, rely=0.5, anchor="center", relwidth=0.94, relheight=0.88)

        self.canvas = tk.Canvas(
            bezel,
            bg="#0b0b0b",
            highlightthickness=0,
            bd=0
        )
        self.canvas.pack(fill="both", expand=True, padx=8, pady=8)
        self.canvas.bind("<Configure>", self.redraw_panel)

        bottom = tk.Frame(self.root, bg="#111111")
        bottom.pack(fill="x", padx=14, pady=(0, 14))

        input_row = tk.Frame(bottom, bg="#111111")
        input_row.pack(fill="x", pady=(6, 8))

        right_input = tk.Frame(input_row, bg="#111111")
        right_input.pack(side="left", expand=True, fill="x", padx=(10, 0))
        tk.Label(
            right_input,
            text="Current [A]",
            fg="#f0f0f0",
            bg="#111111",
            font=("Segoe UI", 12, "bold")
        ).pack(pady=(0, 8))
        tk.Entry(
            right_input,
            textvariable=self.input_current_var,
            justify="center",
            font=("Segoe UI", 12, "bold"),
            validate="key",
            validatecommand=num_vcmd,
            bg="#2a2a2a",
            fg="#f0f0f0"   
        ).pack(ipady=8, fill="x", padx=8)
        
        left_input = tk.Frame(input_row, bg="#111111")
        left_input.pack(side="left", expand=True, fill="x", padx=(0, 10))
        tk.Label(
            left_input,
            text="Voltage [V]",
            fg="#f0f0f0",
            bg="#111111",
            font=("Segoe UI", 12, "bold")
        ).pack(pady=(0, 8))
        tk.Entry(
            left_input,
            textvariable=self.input_voltage_var,
            justify="center",
            font=("Segoe UI", 12, "bold"),
            validate="key",
            validatecommand=num_vcmd,
            bg="#2a2a2a",
            fg="#f0f0f0"            
        ).pack(ipady=8, fill="x", padx=8)
        
        wiper_row = tk.Frame(bottom, bg="#111111")
        wiper_row.pack(fill="x", pady=(10, 0))
        wiper_right = tk.Frame(wiper_row, bg="#111111")
        wiper_right.pack(side="left", expand=True, fill="x", padx=(10, 0))
        wiper_left = tk.Frame(wiper_row, bg="#111111")
        wiper_left.pack(side="left", expand=True, fill="x", padx=(0, 10))
        tk.Label(
            wiper_right,
            textvariable=self.wiper_current_var,
            fg="#f0f0f0",
            bg="#111111",
            font=("Segoe UI", 12, "bold"),
            anchor="center"
        ).pack(fill="x")

        tk.Label(
            wiper_left,
            textvariable=self.wiper_voltage_var,
            fg="#f0f0f0",
            bg="#111111",
            font=("Segoe UI", 12, "bold"),
            anchor="center"
        ).pack(fill="x")


        

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_box["values"] = ports
        if ports:
            if self.port_var.get() not in ports:
                self.port_var.set(ports[0])
        else:
            self.port_var.set("")

    def find_cp2102_port(self):
        for port_info in serial.tools.list_ports.comports():
            if getattr(port_info, "vid", None) == 0x10C4 and getattr(port_info, "pid", None) == 0xEA60:
                return port_info.device

            searchable = " ".join(
                str(getattr(port_info, attr, "") or "")
                for attr in ("description", "manufacturer", "product", "hwid")
            ).lower()
            if "cp2102" in searchable:
                return port_info.device
        return None

    def auto_connect_cp2102(self):
        if self.running:
            return

        cp2102_port = self.find_cp2102_port()
        if cp2102_port:
            self.port_var.set(cp2102_port)
            self.connect()

    def toggle_connection(self):
        if self.running:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("No port", "Please select a COM port.")
            return

        try:
            self.ser = serial.Serial(port, baudrate=115200, timeout=0.2)
            self.running = True
            self.conn_btn.config(text="Disconnect")
            self.top.pack_forget()
            self.read_thread = threading.Thread(target=self.read_loop, daemon=True)
            self.read_thread.start()
        except Exception as e:
            messagebox.showerror("Connection error", str(e))
            self.ser = None
            self.running = False

    def disconnect(self):
        self.running = False
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
        self.ser = None
        self.conn_btn.config(text="Connect")
        if not self.top.winfo_manager():
            self.top.pack(fill="x", padx=14, pady=(12, 8), before=self.shell)

    def validate_numeric(self, value):
        if value == "":
            return True
        if value.count(".") > 1:
            return False
        return value.replace(".", "", 1).isdigit()

    def read_loop(self):
        while self.running and self.ser:
            try:
                start = self.ser.read(1)
                if start != b"M":
                    continue

                payload = self.ser.read(5)
                tail = self.ser.read(2)

                if len(payload) != 5 or tail != b"\r\n":
                    continue

                meas_voltage = payload[0]/100
                meas_current = payload[1]/1000
                meas_power = payload[2]/1000
                wiper_voltage = payload[3]
                wiper_current = payload[4]

                self.root.after(
                    0,
                    self.update_values,
                    meas_voltage,
                    meas_current,
                    meas_power,
                    wiper_voltage,
                    wiper_current
                )

            except Exception:
                self.root.after(0, self.disconnect)
                break

    def update_values(self, v, c, p, wv, wc):
        self.v_var.set(f"{v:05.2f}")
        self.c_var.set(f"{c:05.3f}")
        self.p_var.set(f"{p:05.3f}")
        self.wiper_voltage_var.set(f"{wv}")
        self.wiper_current_var.set(f"{wc}")
        self.redraw_panel()

    def redraw_panel(self, event=None):
        self.canvas.delete("all")

        w = self.canvas.winfo_width()
        h = self.canvas.winfo_height()
        if w < 10 or h < 10:
            return

        self.canvas.create_rectangle(4, 4, w - 4, h - 4, outline="#909090", width=2)
        self.canvas.create_rectangle(10, 10, w - 10, h - 10, outline="#d0d0d0", width=1)

        digit_font = ("Consolas", max(30, int(h * 0.18)), "bold")
        unit_font = ("Georgia", max(16, int(h * 0.08)))
        mark_font = ("Georgia", max(11, int(h * 0.055)))
        red = "#ff2b2b"

        y1 = h * 0.24
        y2 = h * 0.50
        y3 = h * 0.76

        x_num = w * 0.48
        x_unit = w * 0.84

        self.canvas.create_text(x_num, y1, text=self.v_var.get(), fill=red, font=digit_font, anchor="center")
        self.canvas.create_text(x_num, y2, text=self.c_var.get(), fill=red, font=digit_font, anchor="center")
        self.canvas.create_text(x_num, y3, text=self.p_var.get(), fill=red, font=digit_font, anchor="center")

        self.canvas.create_text(x_unit, y1, text="V", fill="#ededed", font=unit_font, anchor="center")
        self.canvas.create_text(x_unit, y2, text="A", fill="#ededed", font=unit_font, anchor="center")
        self.canvas.create_text(x_unit, y3, text="W", fill="#ededed", font=unit_font, anchor="center")



    def on_close(self):
        self.disconnect()
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    style = ttk.Style()
    try:
        style.theme_use("clam")
    except Exception:
        pass
    app = PSUApp(root)
    root.mainloop()
