from http.server import HTTPServer, BaseHTTPRequestHandler
import urllib.parse
import ssl
import argparse
import sys
import signal
import threading

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
        httpd.shutdown()  # Gracefully shut down the server
        httpd.server_close()  # Close the socket
        sys.exit(0)
    return signal_handler


def run(args):
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile=args.certificate, keyfile=args.key)
    context.check_hostname = False

    httpd = HTTPServer((args.address, args.port), HTTPHandler)
    httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

    # Run the server in a separate thread to allow graceful shutdown
    server_thread = threading.Thread(target=httpd.serve_forever)
    server_thread.daemon = True  # Make sure the thread exits when main program exits
    server_thread.start()

    print(f"Serving on https://{args.address}:{args.port}")

    def signal_handler(_signal, _frame):
        print('Stopping server...')
        httpd.shutdown()  # Gracefully shut down the server
        httpd.server_close()  # Close the socket
        print('Server stopped')

    signal.signal(signal.SIGINT, signal_handler)

    try:
        server_thread.join()  # Wait for the server thread to finish
    except KeyboardInterrupt:
        print("Received interrupt, shutting down...")

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
