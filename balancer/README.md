In order to run the balancer, simply use `python3 balancer.py` with appropriate params (see `-h`).

To generate certificate and key:
```bash
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 3650 -nodes -subj "/C=US/ST=Oregon/L=Portland/O=cppfs/OU=Org/CN=www.example.com"
```

If you're running the balancer inside docker container and trying to connect outside of the container, make sure that you're specifying correct container address for the balancer. Example:
```bash
$ docker inspect "${USER}_cppfs" | grep IPAddress
            "SecondaryIPAddresses": null,
            "IPAddress": "172.17.0.2",
                    "IPAddress": "172.17.0.2",
$ python3 balancer.py -a 172.17.0.2 -c /tmp/cert.pem -k /tmp/key.pem
```
