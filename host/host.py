import socket
import os
import sys

HOST = "192.168.1.9"
PORT = 27015

def send(con: socket, msg: any):
    con.sendall(msg)

def recv(con: socket):
    return con.recv(8192)

def rlog(msg: str):
    print(f"[*] {msg}")

def run_req(con: socket, file_path: str):
    if(not os.path.exists(file_path)):
        rlog(f"Build output file not found at '{file_path}'")
        return


    send_file_size = os.path.getsize("main.exe")
    send(con, f"RUN {send_file_size}".encode("utf-8"))

    resp: str = recv(con).decode("utf-8")

    match resp:
        case "ACK":
            rlog("RUN request acknowledged by client. Sending file..")
            send_file = open(file_path, "rb")
        
            send(con, send_file.read()) 
            rlog("File transfer completed.")

            # TODO: add a callback here
            return

        case "NACK":
            rlog("RUN request declined by client.")
            return

def main():
    if(len(sys.argv) != 3):
        print(f"Usage: {sys.argv[0]} 'BUILD_COMMAND' 'OUTPUT_FILE'")
        return

    build_cmd = sys.argv[1]
    output_file = sys.argv[2]

    rlog("Starting 9x-dbg-host..")

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    s.bind((HOST, PORT))
    s.listen()
    
    print("")
    print(f"Server host: {HOST}")
    print(f"Server port: {PORT}")
    print(f"Build command: {build_cmd}")
    print(f"Output file: {output_file}")
    print("")

    while True:
        rlog(f"Waiting for 9x-dbg-runner..")
        con, addr = s.accept()
        hello_msg = con.recv(4096)

        if(not hello_msg.decode("utf-8") == "HELLO"):
            rlog("Unknown hello message.")
            return -1
        
        send(con, "ACK".encode("utf-8"))
        rlog("9x runner connected.")

        while True:
            
            # Add compiler stuff like this
            print("Available Actions: ")
            print(" (S) Send and run current build")
            print(" (B) Build, send and run the project")
            print(" (K) Kill the currently running process")
            print("> ", end="")
            answ: str = input().lower()

            match answ:
                case "s":
                    run_req(con, output_file)

                case "b":
                    rlog("Running build command..")
                    if(not os.system(build_cmd) == 0):
                        rlog("Build command failed.")
                        continue

                    run_req(con, output_file)

                case "k":
                    rlog("Attempting to kill process..")
                    send(con, "SUBKILL".encode("utf-8"))

                    resp: str = recv(con).decode("UTF-8")
                    if(resp == "ACK"):
                        print("Process killed.")
                    else:
                        print("Could not kill process or none is running.")




if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        rlog("Exiting on KeyboardInterrupt")

