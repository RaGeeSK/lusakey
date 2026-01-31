# Lusakey v6.2 - С КНОПКОЙ СОХРАНИТЬ В ДИАЛОГЕ
# pip install cryptography
# python lusakey.py

import tkinter as tk
from tkinter import ttk, messagebox
import secrets
import string
import sqlite3
import time
import os
from pathlib import Path
import hashlib

class Lusakey:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Lusakey v6.2")
        self.root.geometry("1200x800")
        self.root.resizable(True, True)
        
        self.db_path = Path(os.getenv('LOCALAPPDATA')) / "lusakey" / "lusakey.db"
        self.db_path.parent.mkdir(exist_ok=True)
        
        self.master_password = None
        self.is_unlocked = False
        self.passwords = []
        
        self.colors = {
            'bg': '#3d3d3d',
            'header_bg': '#2d2d2d',
            'frame_bg': '#3a3a3a',
            'table_bg': '#4a4a4a',
            'table_row': '#5a5a5a',
            'fg': '#ffffff',
            'orange': '#ff9500',
            'orange_hover': '#ffb347',
            'entry_bg': '#4a4a4a',
            'red': '#e74c3c',
            'red_hover': '#c0392b',
            'green': '#27ae60',
            'green_hover': '#2ecc71'
        }
        
        self.root.configure(bg=self.colors['bg'])
        self.init_db()
        self.create_welcome_screen()
        
    def hash_password(self, password):
        return hashlib.sha256(password.encode()).hexdigest()
    
    def save_master_password(self, hashed_password):
        hash_path = self.db_path.parent / "master.hash"
        with open(hash_path, 'w') as f:
            f.write(hashed_password)
    
    def check_master_password(self, password):
        hash_path = self.db_path.parent / "master.hash"
        if not hash_path.exists():
            return False
        with open(hash_path, 'r') as f:
            saved_hash = f.read().strip()
        return saved_hash == self.hash_password(password)
    
    def init_db(self):
        try:
            self.conn = sqlite3.connect(str(self.db_path))
            cursor = self.conn.cursor()
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS passwords (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    site TEXT NOT NULL,
                    login TEXT NOT NULL,
                    password TEXT NOT NULL,
                    notes TEXT,
                    url TEXT,
                    created TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            ''')
            self.conn.commit()
        except Exception as e:
            print(f"DB Error: {e}")
            self.conn = None
    
    def create_welcome_screen(self):
        for widget in self.root.winfo_children():
            widget.destroy()
        
        main_frame = tk.Frame(self.root, bg=self.colors['frame_bg'])
        main_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        tk.Label(main_frame, text="🔐 Lusakey", font=("Segoe UI", 64, "bold"), 
                fg=self.colors['orange'], bg=self.colors['frame_bg']).pack(pady=120)
        
        tk.Button(main_frame, text="🚪 ВОЙТИ", font=("Segoe UI", 22, "bold"),
                 bg=self.colors['orange'], fg="white", activebackground=self.colors['orange_hover'],
                 relief=tk.FLAT, padx=80, pady=25, bd=0,
                 command=self.show_login_dialog, cursor="hand2").pack(pady=40)
        
        tk.Button(main_frame, text="✨ СОЗДАТЬ", font=("Segoe UI", 22, "bold"),
                 bg=self.colors['orange'], fg="white", activebackground=self.colors['orange_hover'],
                 relief=tk.FLAT, padx=80, pady=25, bd=0,
                 command=self.show_signup_dialog, cursor="hand2").pack(pady=20)
        
        self.status_label = tk.Label(main_frame, text="Готов к работе", font=("Segoe UI", 12), 
                                   fg="#888888", bg=self.colors['frame_bg'])
        self.status_label.pack(side=tk.BOTTOM, pady=60)
    
    def show_login_dialog(self):
        self.login_dialog = tk.Toplevel(self.root)
        self.login_dialog.title("🚪 Вход")
        self.login_dialog.geometry("500x380")
        self.login_dialog.resizable(False, False)
        self.login_dialog.configure(bg=self.colors['frame_bg'])
        self.login_dialog.transient(self.root)
        self.login_dialog.grab_set()
        
        self.center_window(self.login_dialog, 500, 380)
        
        tk.Label(self.login_dialog, text="🔓 Введите мастер-пароль", 
                font=("Segoe UI", 24, "bold"), fg=self.colors['orange'], 
                bg=self.colors['frame_bg']).pack(pady=40)
        
        pass_frame = tk.Frame(self.login_dialog, bg=self.colors['bg'])
        pass_frame.pack(pady=25)
        
        tk.Label(pass_frame, text="Пароль:", font=("Segoe UI", 14), 
                fg="#ffffff", bg=self.colors['bg']).pack(pady=(25, 12))
        
        self.login_entry = tk.Entry(pass_frame, show="*", font=("Segoe UI", 15),
                                   bg=self.colors['entry_bg'], fg="#ffffff",
                                   insertbackground="white", relief=tk.FLAT,
                                   bd=0, highlightthickness=3, highlightcolor=self.colors['orange'])
        self.login_entry.pack(pady=12, ipady=15, fill=tk.X, padx=50)
        self.login_entry.focus()
        
        btn_frame = tk.Frame(self.login_dialog, bg=self.colors['frame_bg'])
        btn_frame.pack(pady=40)
        
        tk.Button(btn_frame, text="🚪 ВОЙТИ", font=("Segoe UI", 14, "bold"),
                 bg=self.colors['orange'], fg="white", activebackground=self.colors['orange_hover'],
                 relief=tk.FLAT, padx=50, pady=16, bd=0,
                 command=self.login).pack(side=tk.LEFT, padx=20)
        
        tk.Button(btn_frame, text="❌ Назад", font=("Segoe UI", 14, "bold"),
                 bg="#606060", fg="white", activebackground="#505050",
                 relief=tk.FLAT, padx=50, pady=16, bd=0,
                 command=self.login_dialog.destroy).pack(side=tk.LEFT, padx=20)
        
        self.login_entry.bind('<Return>', lambda e: self.login())
    
    def show_signup_dialog(self):
        self.signup_dialog = tk.Toplevel(self.root)
        self.signup_dialog.title("✨ Создать аккаунт")
        self.signup_dialog.geometry("550x520")
        self.signup_dialog.resizable(False, False)
        self.signup_dialog.configure(bg=self.colors['frame_bg'])
        self.signup_dialog.transient(self.root)
        self.signup_dialog.grab_set()
        
        self.center_window(self.signup_dialog, 550, 520)
        
        tk.Label(self.signup_dialog, text="✨ СОЗДАТЬ НОВЫЙ АККАУНТ", 
                font=("Segoe UI", 22, "bold"), fg=self.colors['orange'], 
                bg=self.colors['frame_bg']).pack(pady=40)
        
        fields_frame = tk.Frame(self.signup_dialog, bg=self.colors['bg'], relief=tk.RAISED, bd=2)
        fields_frame.pack(pady=20, padx=30, fill=tk.X)
        
        tk.Label(fields_frame, text="Новый мастер-пароль:", font=("Segoe UI", 14, "bold"), 
                fg="#ffffff", bg=self.colors['bg']).pack(pady=(30, 10), anchor=tk.W, padx=30)
        self.signup_pass1 = tk.Entry(fields_frame, show="*", font=("Segoe UI", 15),
                                   bg=self.colors['entry_bg'], fg="#ffffff",
                                   relief=tk.FLAT, insertbackground="white",
                                   highlightthickness=2, highlightcolor=self.colors['orange'])
        self.signup_pass1.pack(pady=5, ipady=15, fill=tk.X, padx=30)
        self.signup_pass1.focus()
        
        tk.Label(fields_frame, text="Подтвердите пароль:", font=("Segoe UI", 14, "bold"), 
                fg="#ffffff", bg=self.colors['bg']).pack(pady=(25, 10), anchor=tk.W, padx=30)
        self.signup_pass2 = tk.Entry(fields_frame, show="*", font=("Segoe UI", 15),
                                   bg=self.colors['entry_bg'], fg="#ffffff",
                                   relief=tk.FLAT, insertbackground="white",
                                   highlightthickness=2, highlightcolor=self.colors['orange'])
        self.signup_pass2.pack(pady=5, ipady=15, fill=tk.X, padx=30)
        
        btn_frame = tk.Frame(self.signup_dialog, bg=self.colors['frame_bg'])
        btn_frame.pack(pady=40)
        
        tk.Button(btn_frame, text="✨ СОЗДАТЬ АККАУНТ", font=("Segoe UI", 15, "bold"),
                 bg=self.colors['orange'], fg="white", activebackground=self.colors['orange_hover'],
                 relief=tk.FLAT, padx=60, pady=20, bd=0,
                 command=self.signup).pack(side=tk.LEFT, padx=25)
        
        tk.Button(btn_frame, text="❌ НАЗАД", font=("Segoe UI", 15, "bold"),
                 bg="#606060", fg="white", activebackground="#505050",
                 relief=tk.FLAT, padx=60, pady=20, bd=0,
                 command=self.signup_dialog.destroy).pack(side=tk.LEFT, padx=25)
        
        self.signup_dialog.bind('<Return>', lambda e: self.signup())
    
    def signup(self):
        password1 = self.signup_pass1.get().strip()
        password2 = self.signup_pass2.get().strip()
        
        if not password1 or not password2:
            messagebox.showerror("Ошибка", "Заполните все поля!")
            return
        
        if password1 != password2:
            messagebox.showerror("Ошибка", "❌ Пароли не совпадают!")
            self.signup_pass2.delete(0, tk.END)
            self.signup_pass2.focus()
            return
        
        if len(password1) < 8:
            messagebox.showerror("Ошибка", "❌ Пароль слишком короткий!\nМинимум 8 символов")
            return
        
        try:
            if self.conn:
                self.conn.close()
            hash_path = self.db_path.parent / "master.hash"
            if hash_path.exists():
                hash_path.unlink()
            if self.db_path.exists():
                self.db_path.unlink()
            
            self.init_db()
            hashed_password = self.hash_password(password1)
            self.save_master_password(hashed_password)
            
            self.master_password = password1
            self.is_unlocked = True
            self.passwords = []
            
            self.signup_dialog.destroy()
            self.load_main_screen()
            
        except Exception as e:
            messagebox.showerror("Ошибка", f"❌ Ошибка создания:\n{str(e)}")
    
    def login(self):
        password = self.login_entry.get().strip()
        if not password:
            messagebox.showerror("Ошибка", "Введите пароль!")
            return
        
        if self.check_master_password(password):
            self.master_password = password
            try:
                self.load_passwords()
                self.is_unlocked = True
                self.login_dialog.destroy()
                self.load_main_screen()
            except Exception as e:
                messagebox.showerror("Ошибка", "Ошибка загрузки базы!")
        else:
            messagebox.showerror("Ошибка", "❌ Неверный мастер-пароль!")
    
    def center_window(self, window, width, height):
        x = (self.root.winfo_screenwidth() // 2) - (width // 2)
        y = (self.root.winfo_screenheight() // 2) - (height // 2)
        window.geometry(f'{width}x{height}+{x}+{y}')
    
    def load_passwords(self):
        if not self.conn:
            return
        cursor = self.conn.cursor()
        cursor.execute("SELECT * FROM passwords ORDER BY site")
        rows = cursor.fetchall()
        self.passwords = []
        for row in rows:
            self.passwords.append({
                'id': row[0], 'site': row[1], 'login': row[2], 
                'password': row[3], 'notes': row[4], 'url': row[5]
            })
    
    def load_main_screen(self):
        for widget in self.root.winfo_children():
            widget.destroy()
        
        header = tk.Frame(self.root, bg=self.colors['header_bg'], height=80)
        header.pack(fill=tk.X, side=tk.TOP)
        header.pack_propagate(False)
        
        left_header = tk.Frame(header, bg=self.colors['header_bg'])
        left_header.pack(side=tk.LEFT, padx=30, pady=15)
        
        tk.Label(left_header, text="🎁", font=("Segoe UI", 32), 
                bg=self.colors['header_bg'], fg=self.colors['orange']).pack(side=tk.LEFT, padx=(0, 15))
        
        tk.Label(left_header, text="ВАШИ ПАРОЛИ", font=("Segoe UI", 24, "bold"), 
                bg=self.colors['header_bg'], fg=self.colors['orange']).pack(side=tk.LEFT)
        
        tk.Button(header, text="🔒 Выйти", font=("Segoe UI", 13, "bold"),
                 bg=self.colors['orange'], fg="white", activebackground=self.colors['orange_hover'],
                 relief=tk.FLAT, padx=30, pady=12, bd=0,
                 command=self.lock_db, cursor="hand2").pack(side=tk.RIGHT, padx=30, pady=20)
        
        main_container = tk.Frame(self.root, bg=self.colors['bg'])
        main_container.pack(fill=tk.BOTH, expand=True)
        
        search_container = tk.Frame(main_container, bg=self.colors['bg'])
        search_container.pack(fill=tk.X, padx=20, pady=20)
        
        search_frame = tk.Frame(search_container, bg=self.colors['header_bg'], relief=tk.FLAT, bd=0)
        search_frame.pack(fill=tk.X)
        
        tk.Label(search_frame, text="🔍 ПОИСК:", font=("Segoe UI", 14, "bold"), 
                bg=self.colors['header_bg'], fg="#ffffff").pack(side=tk.LEFT, padx=20, pady=15)
        
        self.search_entry = tk.Entry(search_frame, font=("Segoe UI", 13),
                                   bg=self.colors['entry_bg'], fg="#ffffff",
                                   insertbackground="white", relief=tk.FLAT, bd=0)
        self.search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 20), pady=15, ipady=8)
        self.search_entry.bind('<KeyRelease>', self.search_passwords)
        
        table_container = tk.Frame(main_container, bg=self.colors['bg'])
        table_container.pack(fill=tk.BOTH, expand=True, padx=20, pady=(0, 20))
        
        style = ttk.Style()
        style.theme_use('default')
        
        style.configure("Gray.Treeview",
                       background=self.colors['table_bg'],
                       foreground="#e0e0e0",
                       fieldbackground=self.colors['table_bg'],
                       borderwidth=0,
                       font=("Segoe UI", 11),
                       rowheight=30)
        
        style.configure("Gray.Treeview.Heading",
                       background=self.colors['header_bg'],
                       foreground="#ffffff",
                       borderwidth=1,
                       relief="flat",
                       font=("Segoe UI", 12, "bold"))
        
        style.map('Gray.Treeview',
                 background=[('selected', self.colors['orange'])],
                 foreground=[('selected', '#ffffff')])
        
        style.map('Gray.Treeview.Heading',
                 background=[('active', '#1a1a1a')])
        
        columns = ('Сайт', 'Логин', 'URL', 'Создан')
        self.tree = ttk.Treeview(table_container, columns=columns, show='headings', 
                                style="Gray.Treeview", selectmode='browse')
        
        self.tree.heading('Сайт', text='Сайт')
        self.tree.heading('Логин', text='Логин')
        self.tree.heading('URL', text='URL')
        self.tree.heading('Создан', text='Создан')
        
        self.tree.column('Сайт', width=280, anchor=tk.W)
        self.tree.column('Логин', width=280, anchor=tk.W)
        self.tree.column('URL', width=350, anchor=tk.W)
        self.tree.column('Создан', width=180, anchor=tk.CENTER)
        
        v_scrollbar = ttk.Scrollbar(table_container, orient=tk.VERTICAL, command=self.tree.yview)
        h_scrollbar = ttk.Scrollbar(table_container, orient=tk.HORIZONTAL, command=self.tree.xview)
        self.tree.configure(yscrollcommand=v_scrollbar.set, xscrollcommand=h_scrollbar.set)
        
        self.tree.grid(row=0, column=0, sticky='nsew')
        v_scrollbar.grid(row=0, column=1, sticky='ns')
        h_scrollbar.grid(row=1, column=0, sticky='ew')
        
        table_container.grid_rowconfigure(0, weight=1)
        table_container.grid_columnconfigure(0, weight=1)
        
        self.tree.bind('<<TreeviewSelect>>', self.on_select)
        self.tree.bind('<Double-1>', lambda e: self.edit_password())
        
        self.tree.tag_configure('oddrow', background='#4a4a4a')
        self.tree.tag_configure('evenrow', background='#555555')
        
        button_panel = tk.Frame(main_container, bg=self.colors['bg'])
        button_panel.pack(fill=tk.X, padx=20, pady=(0, 20))
        
        left_buttons = tk.Frame(button_panel, bg=self.colors['bg'])
        left_buttons.pack(side=tk.LEFT)
        
        tk.Button(left_buttons, text="➕ ДОБАВИТЬ", font=("Segoe UI", 12, "bold"),
                 bg=self.colors['green'], fg="white", activebackground=self.colors['green_hover'],
                 relief=tk.FLAT, padx=25, pady=12, bd=0,
                 command=self.add_password, cursor="hand2").pack(side=tk.LEFT, padx=5)
        
        self.edit_btn = tk.Button(left_buttons, text="✏️ РЕДАКТИРОВАТЬ", 
                                  font=("Segoe UI", 12, "bold"),
                                  bg=self.colors['orange'], fg="white", 
                                  activebackground=self.colors['orange_hover'],
                                  relief=tk.FLAT, padx=25, pady=12, bd=0,
                                  command=self.edit_password, state='disabled', cursor="hand2")
        self.edit_btn.pack(side=tk.LEFT, padx=5)
        
        self.view_btn = tk.Button(left_buttons, text="👁️ ПРОСМОТР", 
                                 font=("Segoe UI", 12, "bold"),
                                 bg=self.colors['orange'], fg="white", 
                                 activebackground=self.colors['orange_hover'],
                                 relief=tk.FLAT, padx=25, pady=12, bd=0,
                                 command=self.view_password, state='disabled', cursor="hand2")
        self.view_btn.pack(side=tk.LEFT, padx=5)
        
        middle_buttons = tk.Frame(button_panel, bg=self.colors['bg'])
        middle_buttons.pack(side=tk.LEFT, padx=30)
        
        self.copy_pass_btn = tk.Button(middle_buttons, text="🔑 КОПИРОВАТЬ ПАРОЛЬ", 
                                     font=("Segoe UI", 12, "bold"),
                                     bg=self.colors['orange'], fg="white", 
                                     activebackground=self.colors['orange_hover'],
                                     relief=tk.FLAT, padx=25, pady=12, bd=0,
                                     command=self.copy_password, state='disabled', cursor="hand2")
        self.copy_pass_btn.pack(side=tk.LEFT, padx=5)
        
        self.copy_login_btn = tk.Button(middle_buttons, text="📧 КОПИРОВАТЬ ЛОГИН", 
                                       font=("Segoe UI", 12, "bold"),
                                       bg=self.colors['orange'], fg="white", 
                                       activebackground=self.colors['orange_hover'],
                                       relief=tk.FLAT, padx=25, pady=12, bd=0,
                                       command=self.copy_login, state='disabled', cursor="hand2")
        self.copy_login_btn.pack(side=tk.LEFT, padx=5)
        
        right_buttons = tk.Frame(button_panel, bg=self.colors['bg'])
        right_buttons.pack(side=tk.RIGHT)
        
        self.delete_btn = tk.Button(right_buttons, text="🗑️ УДАЛИТЬ", 
                                    font=("Segoe UI", 12, "bold"),
                                    bg=self.colors['red'], fg="white", 
                                    activebackground=self.colors['red_hover'],
                                    relief=tk.FLAT, padx=25, pady=12, bd=0,
                                    command=self.delete_password, state='disabled', cursor="hand2")
        self.delete_btn.pack(side=tk.LEFT, padx=5)
        
        status_bar = tk.Frame(self.root, bg=self.colors['header_bg'], height=40)
        status_bar.pack(fill=tk.X, side=tk.BOTTOM)
        status_bar.pack_propagate(False)
        
        self.status_var = tk.StringVar(value=f"✅ Всего паролей: {len(self.passwords)}")
        tk.Label(status_bar, textvariable=self.status_var, font=("Segoe UI", 11), 
                fg="#ffffff", bg=self.colors['header_bg']).pack(side=tk.LEFT, padx=30, pady=10)
        
        self.populate_tree()
    
    def populate_tree(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        
        for idx, pwd in enumerate(self.passwords):
            created_date = time.strftime('%d.%m.%Y', time.localtime())
            tag = 'evenrow' if idx % 2 == 0 else 'oddrow'
            self.tree.insert('', 'end', values=(
                pwd['site'], 
                pwd['login'], 
                pwd['url'] or '-', 
                created_date
            ), tags=(str(pwd['id']), tag))
    
    def search_passwords(self, event=None):
        query = self.search_entry.get().lower()
        for item in self.tree.get_children():
            self.tree.delete(item)
        
        if not query:
            self.populate_tree()
            return
        
        idx = 0
        for pwd in self.passwords:
            if (query in pwd['site'].lower() or 
                query in pwd['login'].lower() or 
                (pwd['url'] and query in pwd['url'].lower())):
                created_date = time.strftime('%d.%m.%Y', time.localtime())
                tag = 'evenrow' if idx % 2 == 0 else 'oddrow'
                self.tree.insert('', 'end', values=(
                    pwd['site'], 
                    pwd['login'], 
                    pwd['url'] or '-',
                    created_date
                ), tags=(str(pwd['id']), tag))
                idx += 1
    
    def lock_db(self):
        self.is_unlocked = False
        self.master_password = None
        self.create_welcome_screen()
    
    def add_password(self):
        self.password_dialog('add')
    
    def edit_password(self):
        selection = self.tree.selection()
        if not selection:
            return
        
        item = self.tree.item(selection[0])
        pwd_id = int(item['tags'][0])
        pwd = next((p for p in self.passwords if p['id'] == pwd_id), None)
        
        if pwd:
            self.password_dialog('edit', pwd)
    
    def view_password(self):
        selection = self.tree.selection()
        if not selection:
            return
        
        item = self.tree.item(selection[0])
        pwd_id = int(item['tags'][0])
        pwd = next((p for p in self.passwords if p['id'] == pwd_id), None)
        
        if not pwd:
            return
        
        view_dialog = tk.Toplevel(self.root)
        view_dialog.title("👁️ Просмотр пароля")
        view_dialog.geometry("600x500")
        view_dialog.configure(bg=self.colors['frame_bg'])
        self.center_window(view_dialog, 600, 500)
        view_dialog.transient(self.root)
        view_dialog.grab_set()
        
        tk.Label(view_dialog, text="👁️ ИНФОРМАЦИЯ О ПАРОЛЕ", 
                font=("Segoe UI", 20, "bold"), 
                fg=self.colors['orange'], 
                bg=self.colors['frame_bg']).pack(pady=25)
        
        info_frame = tk.Frame(view_dialog, bg=self.colors['bg'], relief=tk.RAISED, bd=2)
        info_frame.pack(fill=tk.BOTH, expand=True, padx=30, pady=(0, 20))
        
        fields = [
            ('🌐 Сайт:', pwd['site']),
            ('👤 Логин:', pwd['login']),
            ('🔑 Пароль:', '*' * len(pwd['password'])),
            ('🔗 URL:', pwd['url'] or '-'),
            ('📝 Заметки:', pwd['notes'] or '-')
        ]
        
        for label, value in fields:
            field_frame = tk.Frame(info_frame, bg=self.colors['bg'])
            field_frame.pack(fill=tk.X, padx=20, pady=10)
            
            tk.Label(field_frame, text=label, font=("Segoe UI", 12, "bold"),
                    fg="#ffffff", bg=self.colors['bg'], anchor=tk.W).pack(anchor=tk.W, pady=(5, 2))
            
            tk.Label(field_frame, text=value, font=("Segoe UI", 11),
                    fg="#cccccc", bg=self.colors['bg'], anchor=tk.W, 
                    wraplength=500).pack(anchor=tk.W, pady=(2, 5))
        
        btn_frame = tk.Frame(view_dialog, bg=self.colors['frame_bg'])
        btn_frame.pack(pady=20)
        
        tk.Button(btn_frame, text="🔑 Показать пароль", 
                 font=("Segoe UI", 12, "bold"),
                 bg=self.colors['orange'], fg="white",
                 activebackground=self.colors['orange_hover'],
                 relief=tk.FLAT, padx=25, pady=12,
                 command=lambda: messagebox.showinfo("Пароль", pwd['password'])).pack(side=tk.LEFT, padx=10)
        
        tk.Button(btn_frame, text="❌ Закрыть", 
                 font=("Segoe UI", 12, "bold"),
                 bg="#606060", fg="white",
                 activebackground="#505050",
                 relief=tk.FLAT, padx=25, pady=12,
                 command=view_dialog.destroy).pack(side=tk.LEFT, padx=10)
    
    def delete_password(self):
        selection = self.tree.selection()
        if not selection:
            return
        
        item = self.tree.item(selection[0])
        pwd_id = int(item['tags'][0])
        pwd = next((p for p in self.passwords if p['id'] == pwd_id), None)
        
        if pwd:
            result = messagebox.askyesno(
                "⚠️ Подтверждение удаления",
                f"Вы уверены, что хотите удалить запись?\n\n"
                f"🌐 Сайт: {pwd['site']}\n"
                f"👤 Логин: {pwd['login']}\n\n"
                f"❗ Это действие нельзя отменить!"
            )
            
            if result:
                try:
                    cursor = self.conn.cursor()
                    cursor.execute('DELETE FROM passwords WHERE id = ?', (pwd_id,))
                    self.conn.commit()
                    
                    self.load_passwords()
                    self.populate_tree()
                    self.status_var.set(f"🗑️ Запись удалена! Всего паролей: {len(self.passwords)}")
                    
                except Exception as e:
                    messagebox.showerror("Ошибка", f"❌ Ошибка при удалении:\n{str(e)}")
    
    def copy_login(self):
        selection = self.tree.selection()
        if selection:
            item = self.tree.item(selection[0])
            pwd_id = int(item['tags'][0])
            pwd = next(p for p in self.passwords if p['id'] == pwd_id)
            self.root.clipboard_clear()
            self.root.clipboard_append(pwd['login'])
            self.status_var.set(f"📧 Логин скопирован: {pwd['login']}")
            self.root.after(30000, self.clear_clipboard)
    
    def password_dialog(self, mode='add', password=None):
        dialog = tk.Toplevel(self.root)
        title = "✏️ Редактировать пароль" if mode == 'edit' else "➕ Добавить пароль"
        dialog.title(title)
        dialog.geometry("700x720")
        dialog.configure(bg=self.colors['header_bg'])
        self.center_window(dialog, 700, 720)
        dialog.transient(self.root)
        dialog.grab_set()
        
        # ЗАГОЛОВОК
        header_text = "✏️ РЕДАКТИРОВАТЬ ЗАПИСЬ" if mode == 'edit' else "➕ НОВЫЙ ПАРОЛЬ"
        tk.Label(dialog, text=header_text, font=("Segoe UI", 24, "bold"), 
                fg=self.colors['orange'], bg=self.colors['header_bg']).pack(pady=30)
        
        # КОНТЕЙНЕР ДЛЯ ПОЛЕЙ
        scroll_container = tk.Frame(dialog, bg=self.colors['bg'])
        scroll_container.pack(fill=tk.BOTH, expand=True, padx=30, pady=(0, 20))
        
        fields = ['site', 'login', 'password', 'url', 'notes']
        field_labels = {
            'site': '🌐 Сайт',
            'login': '👤 Логин',
            'password': '🔑 Пароль',
            'url': '🔗 URL',
            'notes': '📝 Заметки'
        }
        entries = {}
        
        for field in fields:
            field_container = tk.Frame(scroll_container, bg=self.colors['bg'])
            field_container.pack(fill=tk.X, pady=10)
            
            label_frame = tk.Frame(field_container, bg=self.colors['bg'])
            label_frame.pack(fill=tk.X)
            
            tk.Label(label_frame, text=field_labels[field] + ":", 
                    font=("Segoe UI", 13, "bold"), 
                    fg="#ffffff", bg=self.colors['bg']).pack(anchor=tk.W, pady=(0, 8))
            
            if field == 'notes':
                entry = tk.Text(field_container, font=("Segoe UI", 12), height=3,
                              bg=self.colors['entry_bg'], fg="#ffffff",
                              relief=tk.FLAT, insertbackground="white",
                              wrap=tk.WORD, padx=10, pady=8)
                entry.pack(fill=tk.X)
                if password and password.get(field):
                    entry.insert('1.0', password[field])
            else:
                show = '*' if field == 'password' else ''
                entry = tk.Entry(field_container, font=("Segoe UI", 13), show=show,
                               bg=self.colors['entry_bg'], fg="#ffffff",
                               relief=tk.FLAT, insertbackground="white")
                entry.pack(fill=tk.X, ipady=10)
                if password:
                    entry.insert(0, password.get(field, ''))
            
            entries[field] = entry
        
        # КНОПКА ГЕНЕРАТОРА
        gen_frame = tk.Frame(dialog, bg=self.colors['header_bg'])
        gen_frame.pack(pady=15)
        
        def generate():
            chars = string.ascii_letters + string.digits + "!@#$%^&*()-_=+[]{}|;:,.<>?"
            pwd = ''.join(secrets.choice(chars) for _ in range(20))
            entries['password'].delete(0, tk.END)
            entries['password'].insert(0, pwd)
            messagebox.showinfo("✅ Пароль сгенерирован", f"Новый пароль:\n\n{pwd}\n\n(скопируйте его сейчас)")
        
        tk.Button(gen_frame, text="🎲 ГЕНЕРАТОР ПАРОЛЯ (20 символов)", 
                 bg=self.colors['green'], fg="white",
                 activebackground=self.colors['green_hover'],
                 relief=tk.FLAT, padx=40, pady=14, 
                 font=("Segoe UI", 13, "bold"),
                 command=generate, cursor="hand2").pack()
        
        # КНОПКИ СОХРАНИТЬ И ОТМЕНА
        def save():
            data = {}
            for field in fields:
                if field == 'notes':
                    data[field] = entries[field].get('1.0', tk.END).strip()
                else:
                    data[field] = entries[field].get().strip()
            
            if not data['site']:
                messagebox.showerror("❌ Ошибка", "Заполните поле 'Сайт'!")
                entries['site'].focus()
                return
            
            if not data['login']:
                messagebox.showerror("❌ Ошибка", "Заполните поле 'Логин'!")
                entries['login'].focus()
                return
            
            if not data['password']:
                messagebox.showerror("❌ Ошибка", "Пароль не может быть пустым!")
                entries['password'].focus()
                return
            
            try:
                cursor = self.conn.cursor()
                if mode == 'add':
                    cursor.execute(
                        'INSERT INTO passwords (site, login, password, notes, url) VALUES (?, ?, ?, ?, ?)',
                        (data['site'], data['login'], data['password'], data['notes'], data['url'])
                    )
                    success_msg = "✅ Пароль успешно добавлен!"
                else:
                    cursor.execute(
                        'UPDATE passwords SET site=?, login=?, password=?, notes=?, url=? WHERE id=?',
                        (data['site'], data['login'], data['password'], data['notes'], data['url'], password['id'])
                    )
                    success_msg = "✅ Пароль успешно обновлен!"
                
                self.conn.commit()
                self.load_passwords()
                self.populate_tree()
                self.status_var.set(f"{success_msg} Всего паролей: {len(self.passwords)}")
                dialog.destroy()
                messagebox.showinfo("Успех", success_msg)
                
            except Exception as e:
                messagebox.showerror("❌ Ошибка", f"Ошибка при сохранении:\n{str(e)}")
        
        btn_frame = tk.Frame(dialog, bg=self.colors['header_bg'])
        btn_frame.pack(pady=25)
        
        save_text = "💾 СОХРАНИТЬ" if mode == 'add' else "💾 СОХРАНИТЬ ИЗМЕНЕНИЯ"
        
        # КНОПКА СОХРАНИТЬ - БОЛЬШАЯ И ЯРКАЯ
        tk.Button(btn_frame, text=save_text, 
                 bg=self.colors['green'], fg="white",
                 activebackground=self.colors['green_hover'], 
                 relief=tk.FLAT, padx=50, pady=18,
                 font=("Segoe UI", 14, "bold"), 
                 command=save, cursor="hand2").pack(side=tk.LEFT, padx=15)
        
        tk.Button(btn_frame, text="❌ ОТМЕНА", 
                 bg="#606060", fg="white",
                 activebackground="#505050", 
                 relief=tk.FLAT, padx=50, pady=18,
                 font=("Segoe UI", 14, "bold"), 
                 command=dialog.destroy, cursor="hand2").pack(side=tk.LEFT, padx=15)
        
        # Горячая клавиша Enter для сохранения
        dialog.bind('<Control-Return>', lambda e: save())
    
    def on_select(self, event):
        state = 'normal' if self.tree.selection() else 'disabled'
        if hasattr(self, 'copy_pass_btn'):
            self.copy_pass_btn.config(state=state)
        if hasattr(self, 'copy_login_btn'):
            self.copy_login_btn.config(state=state)
        if hasattr(self, 'edit_btn'):
            self.edit_btn.config(state=state)
        if hasattr(self, 'view_btn'):
            self.view_btn.config(state=state)
        if hasattr(self, 'delete_btn'):
            self.delete_btn.config(state=state)
    
    def copy_password(self):
        selection = self.tree.selection()
        if selection:
            item = self.tree.item(selection[0])
            pwd_id = int(item['tags'][0])
            pwd = next(p for p in self.passwords if p['id'] == pwd_id)
            self.root.clipboard_clear()
            self.root.clipboard_append(pwd['password'])
            self.status_var.set(f"🔑 Пароль скопирован! Буфер обмена очистится через 30 секунд")
            self.root.after(30000, self.clear_clipboard)
    
    def clear_clipboard(self):
        self.root.clipboard_clear()
        self.status_var.set(f"🧹 Буфер обмена очищен. Всего паролей: {len(self.passwords)}")
    
    def run(self):
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.root.mainloop()
    
    def on_closing(self):
        if self.conn:
            self.conn.close()
        self.root.destroy()

if __name__ == "__main__":
    app = Lusakey()
    app.run()