# Freeradius Log Watcher

Monitors the Freeradius accounting log as well as the radius log in real time. Useful for debug purposes on a small network with a handful of known and expected users. 

````
Tue Jun 18 14:45:25 2024 : ERROR: (87) eap_tls: ERROR: (TLS) Alert read:fatal:certificate expired
Tue Jun 18 14:45:25 2024 : Auth: (87) Login incorrect (eap_tls: (TLS) Alert read:fatal:certificate expired): [username] (from client custom_access_point port 1 cli FE:ED:BE:EF:BA:BE)

Tue Jun 18 14:45:25 2024 : Auth: (87) Login OK: [username] (from client custom_access_point port 1 cli FE:ED:BE:EF:BA:BE)

Acct-Status-Type = Start
User-Name = "User"
NAS-IP-Address = 192.168.158.148
Mobility-Domain-ID = 43690
Event-Timestamp = "Jun 18 2024 14:45:47 CEST"

Acct-Status-Type = Stop
User-Name = "User"
NAS-IP-Address = 192.168.158.148
Mobility-Domain-ID = 43690
Event-Timestamp = "Jun 18 2024 14:46:35 CEST"
Acct-Terminate-Cause = User-Request
````

### Usage

On a fresh install of freeradius you must manually enable logging of authentication attempts.
1. Open  */etc/freeradius/3.0/radiusd.conf*.
2. Find ````log { }```` on line 276 (as of 2024-06-18).
3. Inside that scope find "*auto = no*" on line 328 (as of 2024-06-18) and set it to "*auth = yes*".

Once you have built the binary you have three options.
1. Run the program as root.
2. Set the binary with root SUID and run as a normal user.
3. Change the access permissions of the radius accounting logs.

---

The program will use the default directory paths for debian systems ("*/etc/freeradius/3.0*") if no arguments are provided. You can provide custom directory paths as command line arguments:
* -radacct \<path>
* -radius \<path>

**Example:** *./freerad-log-watcher -radacct /home/user/Desktop/radius -radius /home/user/Desktop/accounting*

----

If you want to change what information is printed to the terminal look at CParser.hpp/cpp.

----

## Compiler Support
https://en.cppreference.com/w/cpp/compiler_support/23

# Build
### Pre steps
* /cppcheck.sh must be executable -> sudo chmod +x cppcheck.sh
* Install cppcheck -> sudo apt install cppcheck
* Install cmake -> sudo apt install cmake

## Building with Ninja
* sudo apt install ninja-build
* cd root
* mkdir build
* cd build
#### Debug Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=*!YOUR-COMPILER!* -DCMAKE_CXX_COMPILER=!YOUR-COMPILER!
#### ReleaseWithDebugInfo Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=*!YOUR-COMPILER!* -DCMAKE_CXX_COMPILER=!YOUR-COMPILER!
#### MinSizeRelease Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_C_COMPILER=*!YOUR-COMPILER!* -DCMAKE_CXX_COMPILER=!YOUR-COMPILER!
#### Release Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=*!YOUR-COMPILER!* -DCMAKE_CXX_COMPILER=!YOUR-COMPILER!

#### Compile and Link
ninja


