# 🖥️ Client–Server System (C++ / UDP / Socekts)

## 📌 Përshkrimi i Projektit
Ky është një projekt klient-server i zhvilluar në C++ duke përdorur UDP sockets (Winsock në Windows).

Qëllimi është të demonstrojë komunikimin në rrjet, menaxhimin e file-ve dhe sistemin e privilegjeve (admin/user).

Serveri menaxhon kërkesat nga klientët dhe ekzekuton operacione mbi file system lokal.

---

## ⚙️ Funksionalitetet

### 🔌 Rrjeti
- UDP socket komunikim
- Multi-client support
- IP + Port connection

### 👤 Login
- admin / 1234
- user / 1111

### 🔐 Privilegje
- Admin: read/write/delete/upload/download
- User: vetëm read

---

## 📂 Komandat

/ list
/ read <filename>
/ upload <filename>
/ download <filename>
/ delete <filename>
/ search <keyword>
/ info <filename>

---

## 🌐 HTTP Stats

http://localhost:8080/stats

Shfaq:
- klientët aktivë
- IP-të
- mesazhet

---

## 🚀 Run

Server:
server.exe

Client:
client.exe

---

## ⚠️ Shënime
- UDP nuk është 100% reliable
- File transfer është bazik
- Nuk ka encryption

---

## 🎯 Qëllimi
- Socket programming në C++
- Client-server komunikim
- File management në rrjet

---

## 👨‍💻 Autori
Projekt edukativ në C++
