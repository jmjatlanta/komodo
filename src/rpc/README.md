## The RPC components

RPC in komodo is basically a thread (thread pool?) that accepts connections and communicates via a protocol.

My goal is to break this into classes so that several objects can coexist. RPC objects should sit on a port and be able to handle requests that comply to a certain protocol.

The constructor requires a context that handles the network communication and the protocol that incoming requests should be passed to.

Rules about config files:
### Directories ###
- `-datadir` should point to the directory where the config file lives, and should override any defaults
- if `-datadir` is not set, the data directory is platform specific.
### Configuration Files ###
- `-conf` should point the the config filename, with or without the path. 
    - If the path is not included, it looks in the data directory (see above).
  - If `-conf` is not set, defaults to `[-ac_name].conf`
    - If `-ac_name` is not set, defaults to the platform-specific `komodo.conf`
- If the configuration file is not found, an attempt is made to make one.
