import os
import sys


def main():
    args = " ".join(sys.argv[1:])
    try:
        size = int(args)
        with open("file.txt", "wb") as f:
            f.write(b' ' * size)
    except ValueError:
        print("You must enter a file size to generate")


if __name__ == "__main__":
    main()