import urllib.parse
import ssl
import argparse
import sys
import signal
import threading
import time
import requests
import yaml
from http.server import HTTPServer, BaseHTTPRequestHandler
import urllib3

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

class Storage:
    def __init__(self, ip, port):
        self.ip = ip
        self.port = port
        self.healthy = True

    def get_url(self):
        return f"https://{self.ip}:{self.port}/ping"

    def ping(self):
        try:
            response = requests.get(self.get_url(), verify=False)
            if response.status_code == 200 and response.text.strip() == "pong":
                print(f"Storage {self.ip}:{self.port} is healthy (pong).")
                return True
            else:
                print(f"Storage {self.ip}:{self.port} did not return pong.")
                return False
        except requests.RequestException as e:
            print(f"Error pinging storage {self.ip}:{self.port}: {e}")
            return False


class HTTPHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"Hello, this is a GET response!")

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        if content_length > 0:
            post_data = self.rfile.read(content_length)
            post_data = urllib.parse.parse_qs(post_data.decode('utf-8'))

            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            response = f"Hello, this is a POST response! You sent: {post_data}"
            self.wfile.write(response.encode('utf-8'))
        else:
            self.send_response(400)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(b"No content in POST request")

    def finish(self):
        if self.wfile:
            try:
                self.wfile.flush()
            except BrokenPipeError as e:
                print(f"Got error flushing the response stream: {e}")
        try:
            self.connection.close()
        except (ssl.SSLError, OSError) as e:
            print(f"Got error closing the connection: {e}")

def signal_handler_factory(httpd):
    def signal_handler(_signal, _frame):
        print('Stopping server...')
        httpd.shutdown()
        httpd.server_close()
        sys.exit(0)
    return signal_handler


def run_health_checks(active_storages, inactive_storages, interval=60):
    while True:
        print("Running health checks on storages...")
        for storage in active_storages[:]:
            if not storage.ping():
                print(f"Storage {storage.ip}:{storage.port} is now inactive.")
                active_storages.remove(storage)
                inactive_storages.append(storage)

        for storage in inactive_storages[:]:
            if storage.ping():
                print(f"Storage {storage.ip}:{storage.port} is now active.")
                inactive_storages.remove(storage)
                active_storages.append(storage)

        time.sleep(interval)

def run(args, active_storages):
    inactive_storages = []
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile=args.certificate, keyfile=args.key)
    context.check_hostname = False

    httpd = HTTPServer((args.address, args.port), HTTPHandler)
    httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

    server_thread = threading.Thread(target=httpd.serve_forever)
    server_thread.daemon = True
    server_thread.start()

    print(f"Serving on https://{args.address}:{args.port}")

    health_check_thread = threading.Thread(target=run_health_checks, args=(active_storages, inactive_storages))
    health_check_thread.daemon = True
    health_check_thread.start()

    signal.signal(signal.SIGINT, signal_handler_factory(httpd))

    try:
        server_thread.join()
    except KeyboardInterrupt:
        print("Received interrupt, shutting down...")

def parse_cl_args():
    parser = argparse.ArgumentParser(
        prog='cppfs-balancer',
        description='Logins clients, redirects connections from clients to cpp-fs nodes',
        epilog=f'Example: python3 {sys.argv[0]} -p 4443 -a localhost -c /tmp/cert.pem -k /tmp/key.pem -y /path/to/config.yaml')
    parser.add_argument('-p', '--port', help='port to bind to', type=int, default=4443)
    parser.add_argument('-a', '--address', help='server address to use', type=str, default='localhost')
    parser.add_argument('-c', '--certificate', help='path to certificate file', type=str, required=True)
    parser.add_argument('-k', '--key', help='path to key file', type=str, required=True)
    parser.add_argument('-y', '--yaml', help='path to YAML config file for storages', type=str, required=True)
    return parser.parse_args()


def load_storages_from_yaml(yaml_path):
    with open(yaml_path, 'r') as yaml_file:
        config = yaml.safe_load(yaml_file)
    storages = []
    if 'storages' in config:
        for storage in config['storages']:
            storages.append(Storage(ip=storage['ip'], port=storage['port']))
    return storages


def main():
    args = parse_cl_args()
    storages = load_storages_from_yaml(args.yaml)
    run(args, storages)


if __name__ == "__main__":
    main()

