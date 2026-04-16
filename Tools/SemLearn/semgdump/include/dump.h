#ifndef __DUMP_H__
#define __DUMP_H__


void StartReadShm ();
char* PopCallGraph ();
void Init (char* DotFile, char* FIdFile, char* StCgDot, unsigned MinSeq);
void ConvertDot (char* StCgDot, char* FIDsFile);







#endif