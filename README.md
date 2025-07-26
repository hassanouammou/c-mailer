# 📧 C-Mailer

> A lightweight terminal-based email client written in C using `libcurl`.

C-Mailer allows you to send emails directly from the terminal using your SMTP credentials. It features a clean CLI interface, colored prompts, persistent credential storage, and simple cross-platform support (Linux & Windows).

---

## 🚀 Features

- ✅ Send emails from the terminal
- ✅ ANSI-colored CLI prompt
- ✅ Persistent credential storage (`~/.C-Mailer/credentials.txt`)
- ✅ Simple setup and usage
- ✅ Cross-platform: Linux & Windows
- ✅ Built-in `help`, `clear`, `whoami`, and more!

---

## 📸 Preview

```bash
   _____      __  __       _ _
  / ____|    |  \/  |     (_) |
 | |   ______| \  / | __ _ _| | ___ _ __
 | |  |______| |\/| |/ _` | | |/ _ \ '__|
 | |____     | |  | | (_| | | |  __/ |
  \_____|    |_|  |_|\__,_|_|_|\___|_|

Welcome, Hassan!

Available commands:
  send    - Compose and send a new email
  whoami  - Show credentials path and info
  clear   - Clear the terminal screen
  help    - Show this help page again
  exit    - Exit the c-mailer program

c-mailer> send
> To: someone@example.com
> Subject: Hello
> Write your message below (type 'exit' to send):
Hello from C-Mailer!
exit
✅ Email sent successfully!
