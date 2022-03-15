#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

//Name: David Manshari
//ID: 307892935

//Name:Nir Abadi 
//ID: 206172546

int main(int argc, char* argv[]) {

	char* file1path = argv[1];
	char* file2path = argv[2];
	int fd1, fd2;


	if (argc != 3) return 1;

	fd1 = open(file1path, O_RDONLY);
	if (fd1 < 0)
		exit(1);

	fd2 = open(file2path, O_RDONLY);
	if (fd2 < 0) {
		close(fd1);
		exit(1);
	}



	char firstBit;
	char secBit;

	while (read(fd1, &firstBit, 1) != 0 && read(fd2, &secBit, 1) != 0)
	{
		if (firstBit != secBit)
		{
			close(fd1);
			close(fd2);
			exit(1);
		}
	}
	close(fd1);
	close(fd2);
	exit(2);

}
