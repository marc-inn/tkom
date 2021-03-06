#include <string>
#include <fstream>

#include "request.hpp"
#include "const.hpp"
#include "log.hpp"

const int METHOD_LEN = 8;

long prepareForWindows(long j) {
	#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && defined(__CYGWIN__)
		 j++;
	#endif
	return j;
}

const std::string methods_array[METHOD_LEN] = {
    "OPTIONS",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT"
};

long request(std::string path, std::string file_name) {

    // kursor odpowiada pozycji sprawdzanego znaku w file
    long request_cursor = 0;
    long file_length = 0;
    char ch;

    // pobierz dlugosc pliku
    std::ifstream file((path+file_name).c_str());
    if (file) {
        file.seekg(0, file.end);
        file_length = file.tellg();
        file.seekg(0, file.beg);
    }

    // petla sprawdzajaca czy w pliku znajduje sie requestLine
    while (request_cursor < file_length) {
        // sprawdz czy od pozycji request_cursor
        // moze rozpoczynac sie request line
        // jesli requestLine() znajdzie requestLine
        // request_line_cursor odpowiada pozycji w pliku za
        //   requestLine
        // jesli nie znajdzie
        //   jest rowna NOT_FOUND
        long request_line_cursor = requestLine (request_cursor, file);

        // Wykryto requestLine
        // sprawdz czy w pliku znajduja sie pola naglowka
        if (request_line_cursor != NOT_FOUND) {
            // header jest opcjonalny - jesli nie jest obecny
            //   to header_cursor rowne jest request_line_cursor
            // w przeciwnym razie odpowiada pozycji za header'em
			do {
                request_line_cursor = requestHeader (request_line_cursor, file);
                file.seekg(request_line_cursor);
                ch = file.get();
			} while (ch != LF && ch != CR);
            // zakoncz petle
            if (request_line_cursor != NOT_FOUND) {
                report (REQUEST_RESULT, SUCCESS);
            }
        }
        // przesun kursor, poniewaz nie znaleziono zadania
        request_cursor++;
    }
    file.close();
    return SUCCESS;
}

long requestLine (long request_line_cursor, std::ifstream &file) {
    long request_uri_cursor = 0;
    long http_version_cursor = 0;

    // sprawdz czy znaleziono metode
    long method_cursor = method (request_line_cursor, file);

    if (method_cursor == NOT_FOUND) {
        // jesli nie znaleziono method zwroc niepowodzenie
        return NOT_FOUND;
    }
	// file.seekg(method_cursor);
    // sprawdz czy za method znajduje sie znak SP
    char ch = file.get();
    if (ch == SP) {
        request_uri_cursor = requestUri (method_cursor+1, file);
        if (request_uri_cursor == NOT_FOUND) {
            // jesli nie znaleziono method zwroc niepowodzenie
            return NOT_FOUND;
        }
    } else {
        // jesli nie znaleziono SP zwroc niepowodzenie
        return NOT_FOUND;
    }
	file.seekg(request_uri_cursor);
    // sprawdz czy za requestUri jest SP
    ch = file.get();

    if (ch == SP) {
        // sprawdzamy poprawnosc httpVersion
		http_version_cursor = httpVersionRequest(request_uri_cursor + 1, file);
        // jesli nie znaleziono zwroc niepowodzenie
        if (http_version_cursor == NOT_FOUND) {
            return NOT_FOUND;
        } else {
            return http_version_cursor;
        }
    } else {
        return NOT_FOUND;
    }
}

long method (long method_cursor, std::ifstream &file) {
    int i=0, j=0;
    std::string method = "";

    for (i=0; i<METHOD_LEN; i++) {
        method = "";
        std::string current_method = methods_array[i];
        file.seekg(method_cursor);
        for (j=0; j<current_method.length(); j++) {
            char ch = file.get();
            if (ch == current_method[j]) {
                method += ch;
            } else {
                break;
            }
        }

        if (j == current_method.length() && j!=0) {
            report(METHOD, method);
            return method_cursor + j;
        }
    }
    return NOT_FOUND;
}

long requestUri (long request_uri_cursor, std::ifstream &file) {
    file.seekg(request_uri_cursor);
    char ch = file.get();
    // wybierz odpowiednia opcje na podstawie pierwszego znaku
    if (ch == '*') {
        return request_uri_cursor + 1;

    } else if (ch == 'h') {
        printf("ddd\n");
        /*long absolute_uri_cursor = absoluteUri(request_uri_cursor, file);
        // nie znaleziono absoluteUri
        if (absolute_uri_cursor == NOT_FOUND) {
        	return NOT_FOUND;
        } else {
        	// zwroc poprawny wynik
        	return absolute_uri_cursor;
        }*/
    } else if (ch == '\/') {
        long abs_path_cursor = absPath(request_uri_cursor, file);
        if (abs_path_cursor == NOT_FOUND) {
            return NOT_FOUND;
        } else {
            return abs_path_cursor;
        }
    } else {
        return NOT_FOUND;
    }
}

long absoluteUri (long absolute_uri_cursor, std::ifstream &file) {
    // sprawdz czy w pliku jest tekst "http://"
/*    std::string http = "aa";//file.getString (absolute_uri_cursor, 7);
    // jesli nie znaleziono zwroc informacje
    if (http.compare("http://") != 0) {
        return NOT_FOUND;
    }
    // przesun kursor poza znaleziony tekst
    absolute_uri_cursor += 7;
    // sprawdz obecnosc host
    long host_cursor = host (absolute_uri_cursor, file);
    // jesli nie znaleziono zwroc informacje
    if (host_cursor == NOT_FOUND) {
        return NOT_FOUND;
    }
    // sprawdz obecnosc absPath, jest to pole opcjonalne
    long abs_path_cursor = absPath (host_cursor, file);
    // jesli za zwroconym kursorem jest ":" sprawdz port
    char ch = 'a';//getChar (abs_path_cursor, file);
    if (ch == ':') {
        ++abs_path_cursor;
        long port_cursor = port (abs_path_cursor, file);
        return port_cursor;
    }
    // zwroc nowa pozycje kursora
    return host_cursor;
*/
	return 0;
}

long absPath (long abs_path_cursor, std::ifstream &file) {
    int j=0;
    std::string abs_path = "";
    char ch;
    do {
        file.seekg(abs_path_cursor+j);
        ch = file.get();
        abs_path += ch;
        j++;
    } while (ch != SP || abs_path_cursor+j == file.end);

    if (abs_path_cursor+j == file.end) {
        return NOT_FOUND;
    } else {
		report(ABS_PATH, abs_path);
        return abs_path_cursor+j-1;
    }
}

long httpVersionRequest (long http_version_cursor, std::ifstream &file) {
    int j=0;
    char ch;
    // sprawdz czy od pozycji kursora znaki w pliku
    //   sa rowne "HTTP/"
    // pobieraj znak po znaku cyfry az do spotkania "."
    //   jesli spotkasz inny znak niz cyfre - zwroc NOT_FOUND
    //   musi byc co najmniej jedna cyfra
    // pobieraj znak po znaku cyfry az do spotkania CRLF
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

	do{
		file.seekg(http_version_cursor + j);
		ch = file.get();
		if ((ch > '9' || ch < '0') && ch != CR && ch != LF) {
			return NOT_FOUND;
		}
		j++;
	} while (ch != LF);
	j = prepareForWindows(j);
	return http_version_cursor + j;
}

long requestHeader(long request_header_cursor, std::ifstream &file) {

    char ch;
    int j=0;
    std::string field_name = "";
    std::string host = "";
    file.seekg(request_header_cursor);
    do {
        ch = file.get();
        if (ch != ':') { field_name += ch; }
		j++;
    } while (ch != ':');

    if (field_name.compare("Host") == 0) {

        j++;
        ch = file.get();
        do {
            ch = file.get();
			if (ch != CR && ch != LF) { host += ch; }
            j++;
        } while (ch != LF);

        report(HOST, host);
        j = prepareForWindows(j);
        return request_header_cursor + j;
	} else {
        file.seekg(request_header_cursor+j);
        do {
            ch = file.get();
			j++;
        } while (ch != LF);
        j = prepareForWindows(j);
        return request_header_cursor + j;
    }
    return NOT_FOUND;

    // (*) pobieraj znaki az do spotkania ":"
    // utworz z nich String field-name
    //   jesli field-name rowne "host" rozpatrz ten przypadek
    //   jesli nie pobieraj znaki za ":", az do spotkania CRLF
    //     jesli kolejny to znow CRLF
    //       zwroc pozycje nowego kursora
    //     jesli to inny znak to wroc do (*)
    // w przypadku niepowodzenia zwroc niezmieniona pozycje
    // request_header_cursor
}
