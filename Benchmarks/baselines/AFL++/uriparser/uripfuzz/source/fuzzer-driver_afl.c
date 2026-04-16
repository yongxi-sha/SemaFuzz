#include <stdio.h>
#include <stdlib.h>
#include <uriparser/Uri.h>

int main(int argc, char *argv[]) {

	if (argc < 2) {
		exit(1);
	}

    UriParserStateA state;
	UriUriA uri;

	state.uri = &uri;
    uriParseUriA(&state, argv[1]);
		
    uriFreeUriMembersA(&uri);

	return 0;
}
