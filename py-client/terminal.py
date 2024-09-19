import requests
import json
import argparse
import urllib3

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

class Terminal:
    def __init__(self, config):
        self.session = {
            'login': 'guest',
            'cwd': '/',
            'protocol': config['fs']['protocol'],
            'host': config['fs']['host'],
            'port': config['fs']['port'],
            'verbose': False,
            'verify_ssl': config['verify_ssl']
        }
        self.history = []
        # Map command names to handler methods with descriptions for help
        self.commands = {
            "login": ("handle_login", "Login to the terminal (not supported yet)"),
            "pwd": ("handle_pwd", "Print current working directory"),
            "cd": ("handle_cd", "Change directory"),
            "ping": ("ping", "Ping the server"),
            "verbose": ("handle_verbose", "Toggle verbose logging"),
            "host": ("handle_host", "Set server host (not supported yet)"),
            "port": ("handle_port", "Set server port (not supported yet)"),
            "info": ("handle_info", "Display session info"),
            "ls": ("handle_ls", "List files in the current directory"),
            "cat": ("handle_cat", "Display file content"),
            "store": ("handle_store", "Store data in a file"),
            "mkdir": ("handle_mkdir", "Create a directory"),
            "history": ("handle_history", "Show command history"),
            "help": ("handle_help", "Show available commands"),
        }

    def get_fs_url(self):
        return f"{self.session['protocol']}://{self.session['host']}:{self.session['port']}"

    def set_login(self, login):
        self.session['login'] = login

    def set_cwd(self, cwd):
        self.session['cwd'] = cwd

    def set_host(self, host):
        self.session['host'] = host

    def set_port(self, port):
        self.session['port'] = port

    def set_verbose(self, verbose):
        self.session['verbose'] = verbose

    def verbose_log(self, string):
        if self.session['verbose']:
            print(string)

    @staticmethod
    def format_ls(entries):
        lines = []
        for entry in entries:
            entry_type = 'd' if entry['type'] == "Directory" else '-'
            size = entry['size']
            name = entry['name']
            lines.append(f"{entry_type} {size:>5} {name}")
        return '\n'.join(lines)

    @staticmethod
    def format_mkdir(entry):
        return f"Directory '{entry['name']}' created in {entry['dir']}"

    @staticmethod
    def format_store(entry):
        return f"File '{entry['name']}' stored in {entry['dir']} with size {entry['size']} bytes"

    def add_to_history(self, new_line):
        self.history.append(new_line)

    def send_query(self, route, method="GET"):
        verify_ssl = self.session['verify_ssl']
        try:
            query = f"{self.get_fs_url()}/{route}"
            self.verbose_log(f"send query '{query}'")
            response = requests.request(method, query, verify=verify_ssl)
            response.raise_for_status()
            return response
        except requests.RequestException as error:
            self.verbose_log(json.dumps(str(error), indent=1))
            return None

    def ping(self):
        response = self.send_query("ping")
        return response.text if response else "Error pinging server"

    def ls(self, path):
        response = self.send_query(f"ls?path={path}")
        return json.dumps(response.json()['entries'], indent=1) if response else "Error executing ls"

    def cat(self, path, offset='', size=''):
        response = self.send_query(f"cat?path={path}&offset={offset}&size={size}")
        return response.text if response else "Error executing cat"

    def store(self, dir, file, data):
        response = self.send_query(f"store?dir={dir}&file={file}&data={data}", method="POST")
        return json.dumps(response.json(), indent=1) if response else "Error executing store"

    def mkdir(self, at, dir):
        response = self.send_query(f"mkdir?at={at}&dir={dir}", method="POST")
        return json.dumps(response.json(), indent=1) if response else "Error executing mkdir"

    def squash_consecutive_slashes(self, path):
        return path.replace('//', '/')

    def handle_login(self, cmds):
        return "Not supported yet"

    def handle_pwd(self, cmds):
        return self.session['cwd']

    def handle_cd(self, cmds):
        if not cmds:
            path = '/'
        else:
            path = cmds[0] if cmds[0].startswith('/') else self.squash_consecutive_slashes(f"{self.session['cwd']}/{cmds[0]}")
        try:
            self.ls(path)
            self.set_cwd(path)
        except Exception as e:
            return str(e)
        return None

    def handle_verbose(self, cmds):
        if not cmds:
            return f"Verbose is {self.session['verbose']}"
        if cmds[0].lower() == 'on':
            verbose = True
        elif cmds[0].lower() == 'off':
            verbose = False
        else:
            return f"Unknown value for verbose: '{cmds[0]}', it should be either 'on' or 'off'"
        self.set_verbose(verbose)
        return f"set verbose to '{verbose}'"

    def handle_host(self, cmds):
        return "Not supported yet"

    def handle_port(self, cmds):
        return "Not supported yet"

    def handle_info(self, cmds):
        return f"Session info: {json.dumps(self.session, indent=1)}"

    def handle_ls(self, cmds):
        path = cmds[0] if cmds and cmds[0].startswith('/') else f"{self.session['cwd']}/{cmds[0]}" if cmds else self.session['cwd']
        response = self.ls(path)
        if "Error" in response:
            return response
        entries = json.loads(response)
        return Terminal.format_ls(entries)

    def handle_cat(self, cmds):
        if not cmds:
            return "Please, specify path to cat"
        path = cmds[0] if cmds[0].startswith('/') else f"{self.session['cwd']}/{cmds[0]}"
        offset = cmds[1] if len(cmds) > 1 else ""
        size = cmds[2] if len(cmds) > 2 else ""
        return self.cat(path, offset, size)

    def handle_store(self, cmds):
        if len(cmds) < 2:
            return "Please, specify file name and data to store"
        file, *data = cmds
        response = self.store(self.session['cwd'], file, ' '.join(data))
        if "Error" in response:
            return response
        entry = json.loads(response)
        return Terminal.format_store(entry)

    def handle_mkdir(self, cmds):
        if len(cmds) != 1:
            return "Please, specify new directory name"
        response = self.mkdir(self.session['cwd'], cmds[0])
        if "Error" in response:
            return response
        entry = json.loads(response)
        return Terminal.format_mkdir(entry)

    def handle_history(self, cmds):
        return '\n'.join(self.history)

    def handle_help(self, cmds):
        # Display all available commands and their descriptions
        help_text = "Available commands:\n"
        for cmd, (_, description) in self.commands.items():
            help_text += f"    {cmd}: {description}\n"
        return help_text

    def process_cmd(self, cmd):
        parts = cmd.split()
        if not parts:
            return None

        self.add_to_history(cmd)

        bin, *cmds = parts
        command_info = self.commands.get(bin)
        if command_info:
            handler_method_name = command_info[0]
            # Use getattr to call the corresponding handler dynamically
            handler_method = getattr(self, handler_method_name)
            return handler_method(cmds)
        else:
            return f"Unknown command: {bin}"

    def run(self):
        print(f"Welcome to Terminal! Logged in as {self.session['login']}.")
        while True:
            try:
                cmd = input(f"{self.session['login']}:{self.session['cwd']} $ ")
                if cmd == "clear":
                    self.history = []
                else:
                    output = self.process_cmd(cmd)
                    if output:
                        print(output)
            except KeyboardInterrupt:
                print("\nExiting terminal...")
                break

def parse_config():
    parser = argparse.ArgumentParser(description='Terminal Configuration')

    parser.add_argument('--protocol', type=str, default='http', help='Filesystem protocol (default: http)')
    parser.add_argument('--host', type=str, default='localhost', help='Filesystem host (default: localhost)')
    parser.add_argument('--port', type=int, default=8080, help='Filesystem port (default: 8080)')
    parser.add_argument('--verify-ssl', action='store_true', help='Disable SSL certificate verification')

    args = parser.parse_args()

    config = {
        "fs": {
            "protocol": args.protocol,
            "host": args.host,
            "port": args.port,
        },
        "verify_ssl": args.verify_ssl
    }

    return config

if __name__ == "__main__":
    config = parse_config()
    terminal = Terminal(config)
    terminal.run()

