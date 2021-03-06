#include <string>
#include <cstdlib>
#include <fstream>

#include "request.hpp"
#include "response.hpp"
#include "const.hpp"
#include "log.hpp"

const int STATUS_CODE_LEN = 39;
const std::string status_code_array[STATUS_CODE_LEN] = {
    "100", "101",
    "200", "201", "202", "203", "204", "205", "206",
    "300", "302", "303", "304", "305", "307",
    "400", "401", "402", "403", "404", "405", "406", "407", "408",
    "409", "410", "411", "412", "413", "414", "415", "416", "417",
    "500", "501", "502", "503", "504", "505" };

long prepareForWindows1(long j) {
	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && defined(__CYGWIN__)
		// j++;
	#endif
	return j;
}

long response(std::string path, std::string file_name) {

	long status_line_cursor = 0;
	// kursor odpowiada pozycji sprawdzanego znaku w file
	long response_cursor = 0;
    long file_length = 0;
	char ch;

    std::ifstream file((path+file_name).c_str(), std::ios::binary);
    if (file) {
        file.seekg(0, file.end);
        file_length = file.tellg();
        file.seekg(0, file.beg);
    }
	// petla sprawdzajaca czy w pliku znajduje sie Response-Line
	while (response_cursor < file_length) {
		// sprawdz czy od pozycji response_cursor
		// moze rozpoczynac sie statusLine
		// jesli statusLine() znajdzie statusLine
		//   status_line_cursor odpowiada pozycji w pliku za
		//   statusLine
		// jesli nie znajdzie
		//   jest rowna NOT_FOUND
		long status_line_cursor = statusLine (response_cursor, file);

		// Wykryto statusLine
		// sprawdz czy w pliku znajduja sie pola naglowka
		if (status_line_cursor != NOT_FOUND) {

			// header jest opcjonalny - jesli nie jest obecny
			//   to header_cursor rowne jest status_line_cursor
			// w przeciwnym razie odpowiada pozycji za header'em

			do {
                status_line_cursor = responseHeader (status_line_cursor, file);
                file.seekg(status_line_cursor);
				ch = file.get();
            } while (ch != LF && ch != CR);

	        if (status_line_cursor != NOT_FOUND) {
                char ch = file.get();
				if (ch == LF) {
			        messageBody(status_line_cursor + 2, file);
		            report (RESPONSE_RESULT, SUCCESS);
                } else if (ch != LF && ch != NOT_FOUND) {
                    file.seekg(status_line_cursor + 2);
					ch = file.get();
                    printf("Insise: %c\n", ch);
			        messageBody(status_line_cursor + 2, file);
		            report (RESPONSE_RESULT, SUCCESS);
                }
            }
		}
		// przesun kursor, poniewaz nie znaleziono zadania
		response_cursor++;
	}
    report(FINISH, file);
    file.close();
    return 0;
}

long statusLine (long status_line_cursor, std::ifstream &file) {
	long reason_phrase_cursor = 0;
	long status_code_cursor = 0;
	// sprawdz czy znaleziono metode
	long http_version_cursor = httpVersionResponse (status_line_cursor, file);
	// zwroc niepowodzenie w razie nie znalezienia
	if (http_version_cursor == NOT_FOUND) {
		// jesli nie znaleziono Method zwroc niepowodzenie
		return NOT_FOUND;
	}
	file.seekg(http_version_cursor);
	// sprawdz czy za Method znajduje sie znak SP
	char ch = file.get();
	if (ch == SP) {
		status_code_cursor = statusCode (http_version_cursor + 1, file);
		if (status_code_cursor == NOT_FOUND) {
			// jesli nie znaleziono statusCode zwroc niepowodzenie
			return NOT_FOUND;
		}
	} else {
		// jesli nie znaleziono SP zwroc niepowodzenie
		return NOT_FOUND;
	}
	file.seekg(status_code_cursor);
	// sprawdz czy za statusCode jest SP
	ch = file.get();
	if (ch == SP) {
		// sprawdzamy poprawnosc reasonPhrase
		reason_phrase_cursor = reasonPhrase (status_code_cursor + 1, file);
		// jesli nie znaleziono zwroc niepowodzenie
		if (reason_phrase_cursor == NOT_FOUND) {
			return NOT_FOUND;
        } else {
            return reason_phrase_cursor;
        }
	} else {
		// nie znaleziono znaku SP - niepowodzenie
		return NOT_FOUND;
	}
}

long httpVersionResponse (long http_version_cursor, std::ifstream &file) {
    int j=0;
    char ch;

	// sprawdz czy od pozycji kursora znaki w pliku
	//   sa rowne "HTTP/"
	// pobieraj znak po znaku cyfry az do spotkania "."
	//   jesli spotkasz inny znak niz cyfre - zwroc NOT_FOUND
	//   musi byc co najmniej jedna cyfra
	// pobieraj znak po znaku cyfry az do spotkania SP
	//   analogicznie jak wczesniej

	std::string http = "HTTP/";

    for (j=0; j<http.length(); j++) {
        file.seekg(http_version_cursor+j);
        ch = file.get();
        if (ch != http[j]) {
            return NOT_FOUND;
        }
    }

    do {
        file.seekg(http_version_cursor+j);
        ch = file.get();
        if ((ch > '9' || ch < '0') && ch != '.') {
            return NOT_FOUND;
        }
        j++;
    } while (ch != '.');

    do {
        file.seekg(http_version_cursor+j);
        ch = file.get();
        if ((ch > '9' || ch < '0') && ch != SP) {
            return NOT_FOUND;
        }
        j++;
    } while (ch != SP);

    return http_version_cursor + j - 1;
}

long statusCode (long status_code_cursor, std::ifstream &file) {

    int i=0, j=0;
    std::string code = "";

    for (i=0; i<STATUS_CODE_LEN; i++) {
        code = "";
        std::string current_code = status_code_array[i];
        for (j=0; j < current_code.length(); j++) {
            file.seekg(status_code_cursor+j);
            char ch = file.get();
            if (ch == current_code[j]) {
                code += ch;
            } else {
                break;
            }
        }

        if (j == current_code.length()) {
            report(STATUS_CODE, code);
            return status_code_cursor+j;
        }
    }
    return NOT_FOUND;
}

long reasonPhrase (long request_uri_cursor, std::ifstream &file) {
	// pobieraj znaki z file poczawszy od request_uri_cursor
	//   az do napotkania CR LF
	// zwroc pozycje za ostatnim pobranym znakiem
	int j = 0;
	char ch;

    do {
        file.seekg(request_uri_cursor+j);
        ch = file.get();
		if (ch == -1) return NOT_FOUND;
        j++;
    } while (ch != LF);
	j = prepareForWindows1(j);
	return request_uri_cursor + j;
}

long responseHeader(long response_header_cursor, std::ifstream &file) {
	// (*) pobieraj znaki az do spotkania ":"
	// utworz z nich String field_name
	//   jesli field-name rowne "Content_Length" rozpatrz ten przypadek
	//   jesli nie pobieraj znaki za ":", az do spotkania CRLF
	//     jesli kolejny to znow CRLF
	//       zwroc pozycje nowego kursora
	//     jesli to inny znak to wroc do (*)
	// zwroc pozycje nowego kursora
	// w przypadku niepowodzenia zwroc niezmieniona pozycje
	// responseHeader_cursor
	char ch;
    int j=0;
    std::string field_name = "";
    std::string content_length = "";
    std::string content_type = "";
    file.seekg(response_header_cursor);
    do {
        ch = file.get();
        if (ch != ':') {
            field_name += ch;
        }
        j++;
    } while (ch != ':');

    if (field_name.compare("Content-Length") == 0) {
        j++;
        ch = file.get();
        do {
            ch = file.get();
			if (ch != CR && ch != LF) { content_length += ch; };
            j++;
        } while (ch != LF);

        j = prepareForWindows1(j);
        report (CONTENT_LENGTH, content_length);
        return response_header_cursor + j;
    } else if(field_name.compare("Content-Type") == 0) {
        j++;
        ch = file.get();
        bool ready = false;
        do {
            ch = file.get();
            if (ch == ';') { ready = false; }
			if (ch != CR && ch != LF && ready) { content_type += ch; }
            if (ch == '\/') { ready = true; }
            j++;
        } while (ch != LF);

        j = prepareForWindows1(j);
        report (EXTENSION, content_type);
        return response_header_cursor + j;
    } else {
        file.seekg(response_header_cursor+j);
        do {
            ch = file.get();
			if (ch == -1) return NOT_FOUND;
            j++;
        } while (ch != LF);
        j = prepareForWindows1(j);
		return response_header_cursor + j;
    }
	return NOT_FOUND;
}

void messageBody(long message_body_cursor, std::ifstream &file) {

	// utworz tablice bajtow
	//   od message_body_cursor az do konca pliku
	// utworz z tej tablicy plik np. wysylajac tablice
	//   do funkcji logujacej, ktora bedzie to obslugiwala
    //

    printf ("AAAA %d\n", message_body_cursor);

	report (CREATE_FILE, message_body_cursor);
}
