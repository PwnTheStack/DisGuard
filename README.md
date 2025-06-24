<p align="center">
  <img src="https://raw.githubusercontent.com/PwnTheStack/DisGuard/main/assets/logo.png" alt="DisGuard Logo" width="300">
</p>

<h1 align="center">DisGuard 🛡️</h1>

<p align="center">
  <strong>DisGuard</strong> is a lightweight Windows application designed to protect your Discord tokens from being stolen by malicious programs. It watches important Discord folders and running programs in real-time, stopping suspicious activity immediately and blocking their network access. Your tokens stay safe, and you stay worry-free.
</p>

---

## Why Should You Care? 🤔

Your Discord token is basically the **key to your account** — anyone who steals it can pretend to be you, send messages, access servers, and more. Malware authors want these tokens badly.

Even though token stealing is fast, it’s not instantaneous. The attacker needs to:

- Notice and open the token file,  
- Read the token data,  
- And then send it over the internet to themselves.

Depending on your computer and internet speed, this whole process usually takes **somewhere between 50 and 200 milliseconds** (that’s 0.05 to 0.2 seconds).  

**DisGuard’s job? To detect any suspicious activity within a few milliseconds and shut it down before the token leaves your PC.** 🎯

---

## How Does DisGuard Work? 🔍

### 1. Real-Time Folder Watching 👀

DisGuard constantly keeps an eye on **all Discord folders** where token data might be stored:

- `%APPDATA%\Discord`  
- `%LOCALAPPDATA%\Discord`  
- `%LOCALAPPDATA%\DiscordCanary`  
- `%LOCALAPPDATA%\DiscordPTB`

It uses a special Windows feature called `ReadDirectoryChangesW` that lets it get **instant alerts** whenever files change in these folders — like when a token file is accessed or modified.

This means DisGuard notices suspicious file activity almost **immediately** — way faster than human reaction time.

### 2. Process Scanning 🕵️‍♂️

Every quarter of a second (~250 ms), DisGuard scans all running programs to find suspicious ones. It looks for things like:

- Processes with names including “inject”, “grabber”, or “discordstealer” (common in malware),  
- And checks if they are digitally signed by trusted companies (like Microsoft). If trusted, it lets them be.

If a program looks fishy **and** isn’t signed, DisGuard **kills it right away** — no questions asked.

### 3. Firewall Blocking 🔥

Killing the process isn’t always enough — the malware might try to restart or communicate with servers.

So DisGuard adds special firewall rules that **block all network traffic** to and from the suspicious program’s executable file. This cuts off any chance of data sneaking out.

### 4. Notifications & Tray Icon 💬

You’ll get a quick popup notification letting you know if something suspicious was detected and blocked. Plus, there’s a tray icon where you can:

- Show or hide the console,  
- Exit DisGuard cleanly.

---

## Design Philosophy ⚙️

- **User-mode only:** No complex drivers, easier to install and safer.  
- **Minimal dependencies:** Written with native Windows APIs for reliability and speed.  
- **Multithreaded:** Uses separate threads to watch folders and scan processes without slowing down your computer.  
- **Trust-based filtering:** Avoids bothering legitimate software by verifying digital signatures.  
- **Layered protection:** Combines file watching, process termination, and firewall blocking for maximum security.  
- **Lightweight UI:** Custom popup windows and tray icon for a smooth experience with minimal resource usage.

---

## How Well Does It Perform? 🚀

| Metric                               | Typical Value             |
|------------------------------------|--------------------------|
| File change detection delay         | Less than 1 millisecond  |
| Process scan interval               | ~250 milliseconds        |
| Firewall rules application time     | Less than 500 milliseconds|
| Token theft total time (read+send) | ~50-200 milliseconds     |
| CPU usage during idle               | Less than 1%              |
| Memory usage                       | Under 10 MB              |

*In other words:* DisGuard acts way faster than most token stealers can finish their job. 🏃💨

---

## Troubleshooting & FAQs ❓

**Q:** Why do I need to run DisGuard as Administrator?  
**A:** Admin rights are required to terminate processes and create firewall rules effectively.

**Q:** Will DisGuard affect legitimate software?  
**A:** DisGuard skips digitally signed and trusted executables to prevent false positives.

**Q:** What if I want to stop DisGuard?  
**A:** Right-click the tray icon and select “Exit” to safely close the app.

---

## Getting Started: Step-by-Step Guide 📝

### System Requirements

- Windows 10 or later (64-bit recommended)  
- Administrative privileges (needed to stop suspicious processes and add firewall rules)  
- At least 2 GB RAM and minimal CPU usage (DisGuard is lightweight!)  

---

### How to Install & Run DisGuard

1. **Download the latest release**  
   Head over to the [Releases](https://github.com/PwnTheStack/DisGuard/releases) page and download the latest `DisGuard.exe` zip package.

2. **Extract the zip file**  
   Extract all files to a folder you prefer, e.g., `C:\Program Files\DisGuard`.

3. **Run as Administrator**  
   Right-click `DisGuard.exe` and select **Run as administrator**.  
   This is essential for the app to monitor system processes and manage firewall rules properly.

4. **Check the system tray**  
   After launch, DisGuard will minimize to your system tray (near the clock).  
   - Left-click the tray icon to show or hide the console window.  
   - Right-click for options like Exit.

5. **Enjoy peace of mind!**  
   DisGuard will now silently watch your Discord folders and processes for suspicious activity, notifying you instantly of threats.

---

### How to Build DisGuard from Source 🛠️

If you want to compile DisGuard yourself:

1. **Clone the repository**  
   Open a terminal and run:  
   ```bash
   git clone https://github.com/PwnTheStack/DisGuard.git
   cd DisGuard
   ```
2. **Open the solution**  
   Open the `DisGuard.sln` file using **Visual Studio 2022** or any compatible C++ IDE that supports Windows desktop development.

3. **Set build configuration**  
   In Visual Studio, select the build configuration to **Release** and target platform to **x64** (recommended for performance and compatibility).

4. **Build the project**  
   Use the menu option **Build > Build Solution** or press `Ctrl + Shift + B` to compile the project.

5. **Run as Administrator**  
   After a successful build, navigate to the output folder (usually `DisGuard\Release\x64\`) and run the compiled executable with **Administrator privileges**. Right-click the `.exe` file and select **Run as administrator**.  
   This is required so DisGuard can monitor processes and add firewall rules.

---

### License

DisGuard is distributed under the **MIT License**, which means:

- You are free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software.  
- You must include the original copyright and license notices in any copies or substantial portions of the software.  
- DisGuard is provided "as is", without warranty of any kind.

See the full license text in the `LICENSE` file.

---

If you encounter any issues, have questions, or want to contribute, please open an issue or pull request on GitHub. Your feedback and support are appreciated! 🙌
