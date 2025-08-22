import sys

def next_line(file):
    x = file.readline()
    return x.rstrip("\n")


def skip_prolog(file):
    while True:
        line = next_line(file)
        if line == "***START***":
            return


DELIM = ": "


def parse_line(file):
    while True:
        line = next_line(file)
        if line == "***FINISH***":
            return None
        result = line.split(DELIM, maxsplit=1)
        if len(result) != 2:
            sys.exit(f"Not len 2 {result}")
        if result[1] == "CACHE":
            continue
        return tuple(result)


def main():
    with open("c:/users/sigma/documents/docache.txt", "r") as docache:
        with open("c:/users/sigma/documents/nocache.txt", "r") as nocache:
            skip_prolog(docache)
            skip_prolog(nocache)

            do = parse_line(docache)
            no = parse_line(nocache)

            while True:
                if do is None and no is None:
                    break
                if do[0] == no[0]:
                    if do[1] != no[1]:
                        print(f"Mismatch at {do[0]}")
                    do = parse_line(docache)
                    no = parse_line(nocache)
                elif no[0].startswith(do[0]):
                    no = parse_line(nocache)
                else:
                    print (f"do={do[0]}")
                    print (f"no={no[0]}")
                    sys.exit("Bad Compare")
            print ("Logs compare okay")


if __name__ == "__main__":
    main()
