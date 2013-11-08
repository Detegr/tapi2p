## tapi2p
OpenSSL learning project.
The ultimate goal is to create a secure f2f chat/file sharing network.

### Encryption
Encryption is done with OpenSSL. For authentication, 4096bit RSA-keypairs will be used. All data will be end-to-end encrypted with 256bit AES-cbc encryption.

### UI
There will be multiple UI's, including pure command line client, a curses-based cli and web-ui communicating with tapi2p core through a simple websocket server.

### OS support
Tested on Linux. Should be easy to port to platforms with unix socket support.
