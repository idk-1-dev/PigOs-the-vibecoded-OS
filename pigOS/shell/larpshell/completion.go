package main

import (
	"os"
	"path/filepath"
	"strings"
)

var completionScripts = map[string]func(string) []string{
	"git": func(partial string) []string {
		var matches []string
		commands := []string{"status", "add", "commit", "push", "pull", "clone", "checkout", "branch", "merge", "log", "diff", "fetch", "init", "remote", "stash", "reset", "rebase", "tag", "cherry-pick", "config", "blame", "show"}
		for _, c := range commands {
			if strings.HasPrefix(c, partial) {
				matches = append(matches, c)
			}
		}
		return matches
	},
	"npm": func(partial string) []string {
		var matches []string
		commands := []string{"install", "start", "test", "run", "init", "publish", "update", "audit", "cache", "ci", "login", "logout", "outdated", "view", "ls", "uninstall", "link", "unlink", "pack", "dedupe"}
		for _, c := range commands {
			if strings.HasPrefix(c, partial) {
				matches = append(matches, c)
			}
		}
		return matches
	},
	"make": func(partial string) []string {
		return []string{"all", "clean", "install", "run", "debug", "test", "help", "distclean", "rebuild"}
	},
	"docker": func(partial string) []string {
		var matches []string
		commands := []string{"run", "build", "pull", "push", "ps", "images", "exec", "logs", "stop", "start", "rm", "rmi", "inspect", "network", "volume", "compose", "container", "image", "system", "stats", "info", "version"}
		for _, c := range commands {
			if strings.HasPrefix(c, partial) {
				matches = append(matches, c)
			}
		}
		return matches
	},
	"ls": func(partial string) []string {
		var matches []string
		flags := []string{"-l", "-a", "-la", "-h", "-R", "-t", "-S", "-r", "-i", "-n", "-o", "-g", "-p", "-d", "-F"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
	"grep": func(partial string) []string {
		var matches []string
		flags := []string{"-i", "-v", "-n", "-r", "-l", "-c", "-w", "-x", "-E", "-F", "-A", "-B", "--color", "-e", "-f", "-o"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
	"apt": func(partial string) []string {
		var matches []string
		commands := []string{"update", "upgrade", "install", "remove", "purge", "search", "show", "list", "autoremove", "autoclean", "clean", "dist-upgrade", "edit-sources", "policy"}
		for _, c := range commands {
			if strings.HasPrefix(c, partial) {
				matches = append(matches, c)
			}
		}
		return matches
	},
	"apt-get": func(partial string) []string {
		var matches []string
		commands := []string{"update", "upgrade", "install", "remove", "purge", "source", "build-dep", "download", "check", "clean", "autoclean", "autoremove"}
		for _, c := range commands {
			if strings.HasPrefix(c, partial) {
				matches = append(matches, c)
			}
		}
		return matches
	},
	"python": func(partial string) []string {
		var matches []string
		flags := []string{"-m", "-c", "-v", "-u", "-i", "-O", "-OO", "-S", "-E", "-R", "-W", "-x", "-3", "-2"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
	"python3": func(partial string) []string {
		var matches []string
		flags := []string{"-m", "-c", "-v", "-u", "-i", "-O", "-OO", "-S", "-E", "-R", "-W", "-x"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
	"go": func(partial string) []string {
		var matches []string
		commands := []string{"run", "build", "test", "install", "get", "mod", "fmt", "vet", "clean", "version", "help", "init", "generate", "doc", "list", "tool"}
		for _, c := range commands {
			if strings.HasPrefix(c, partial) {
				matches = append(matches, c)
			}
		}
		return matches
	},
	"cargo": func(partial string) []string {
		var matches []string
		commands := []string{"build", "run", "test", "check", "doc", "new", "init", "add", "remove", "update", "publish", "fmt", "clippy", "bench", "clean", "tree"}
		for _, c := range commands {
			if strings.HasPrefix(c, partial) {
				matches = append(matches, c)
			}
		}
		return matches
	},
	"curl": func(partial string) []string {
		var matches []string
		flags := []string{"-X", "-H", "-d", "-o", "-O", "-L", "-I", "-u", "-A", "-b", "-c", "-k", "-s", "-v", "-f", "-m", "-x"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
	"wget": func(partial string) []string {
		var matches []string
		flags := []string{"-O", "-q", "-c", "-r", "-np", "-nH", "-P", "-l", "-A", "-R", "-I", "-E", "-k", "-p", "--no-check-certificate"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
	"chmod": func(partial string) []string {
		var matches []string
		flags := []string{"-R", "-f", "-v", "-c", "-h", "-H", "-L", "-P", "--reference", "--changes", "--no-preserve-root", "--preserve-root"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
	"chown": func(partial string) []string {
		var matches []string
		flags := []string{"-R", "-f", "-v", "-c", "-h", "-H", "-L", "-P", "--reference", "--changes", "--no-preserve-root", "--preserve-root", "-a"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
	"systemctl": func(partial string) []string {
		var matches []string
		commands := []string{"start", "stop", "restart", "reload", "status", "enable", "disable", "is-active", "is-enabled", "daemon-reload", "list-units", "list-unit-files"}
		for _, c := range commands {
			if strings.HasPrefix(c, partial) {
				matches = append(matches, c)
			}
		}
		return matches
	},
	"journalctl": func(partial string) []string {
		var matches []string
		flags := []string{"-u", "-f", "-n", "-xe", "-o", "-r", "-S", "-U", "--since", "--until", "--no-pager", "-a", "-b", "-k"}
		for _, f := range flags {
			if strings.HasPrefix(f, partial) {
				matches = append(matches, f)
			}
		}
		return matches
	},
}

func loadCompletionScripts() {
	bashCompDir := "/usr/share/bash-completion/completions"
	if _, err := os.Stat(bashCompDir); err == nil {
		entries, err := os.ReadDir(bashCompDir)
		if err == nil {
			for _, entry := range entries {
				name := entry.Name()
				if strings.HasSuffix(name, ".bash") {
					cmd := strings.TrimSuffix(name, ".bash")
					if _, ok := completionScripts[cmd]; !ok {
						completionScripts[cmd] = func(partial string) []string {
							return []string{}
						}
					}
				}
			}
		}
	}
	compDir := filepath.Join(filepath.Dir(os.Args[0]), "completions")
	if _, err := os.Stat(compDir); err == nil {
		entries, err := os.ReadDir(compDir)
		if err == nil {
			for _, entry := range entries {
				name := entry.Name()
				if strings.HasSuffix(name, ".bash") {
					cmd := strings.TrimSuffix(name, ".bash")
					if _, ok := completionScripts[cmd]; !ok {
						completionScripts[cmd] = func(partial string) []string {
							return []string{}
						}
					}
				}
			}
		}
	}
}
