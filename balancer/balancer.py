from http.server import HTTPServer, BaseHTTPRequestHandler
import urllib.parse
import ssl
import argparse
import sys
import signal

class HTTPHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"Hello, this is a GET response!")

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        post_data = urllib.parse.parse_qs(post_data.decode('utf-8'))

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        response = f"Hello, this is a POST response! You sent: {post_data}"
        self.wfile.write(response.encode('utf-8'))

    def finish(self):
        if self.wfile:
            try:
                self.wfile.flush()
            except BrokenPipeError as e:
                print(f"Got error flushing the response stream: {e}")
        try:
            # Explicitly unwrap the SSL connection to send the close_notify alert
            self.connection = self.connection.unwrap()
        except (ssl.SSLError, OSError) as e:
            print(f"Got error unwrapping the connection: {e}")
        finally:
            self.connection.close()

def signal_handler(_signal, _frame):
    print('Stopping server...')
    sys.exit(0)

def run(args):
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile=args.certificate, keyfile=args.key)
    context.check_hostname = False

    signal.signal(signal.SIGINT, signal_handler)

    with HTTPServer((args.address, args.port), HTTPHandler) as httpd:
        httpd.socket = context.wrap_socket(httpd.socket, server_side=True)
        httpd.serve_forever()

def parse_cl_args():
    parser = argparse.ArgumentParser(
            prog='cppfs-balancer',
            description='Logins clients, redirects connections from clients to cpp-fs nodes',
            epilog=f'Example: python3 {sys.argv[0]} -p 4443 -a localhost -c /tmp/cert.pem -k /tmp/key.pem')
    parser.add_argument('-p', '--port', help='port to bind to', type=int, default=4443)
    parser.add_argument('-a', '--address', help='server address to use', type=str, default='localhost')
    parser.add_argument('-c', '--certificate', help='path to certificate file', type=str, required=True)
    parser.add_argument('-k', '--key', help='path to key file', type=str, required=True)
    return parser.parse_args()

def main():
    args = parse_cl_args()
    run(args)

if __name__ == "__main__":
    main()
