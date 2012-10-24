## tapi2p
OpenSSL learning project.
Goal is to create a secure f2f chat/file sharing network. As file sharing will get very complicated, f2f chat is the primary goal for now.

### Encryption
Encryption is done with OpenSSL. For authentication, 4096bit RSA-keypairs will be used. All data will be end-to-end encrypted with 256bit AES-cbc encryption.

### UI
UI is done with ncurses. Never done anything with that either :)

### OS support
Different from my other projects, this will mainly focus Linux only. Porting is possible but I'm personally not intrested in doing that.