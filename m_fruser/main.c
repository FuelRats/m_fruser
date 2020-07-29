#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

struct url_data {
    size_t size;
    char* data;
};

size_t write_data (void *ptr, size_t size, size_t nmemb, struct url_data *data) {
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;

    tmp = realloc(data->data, data->size + 1);
    if (tmp) {
        data->data = tmp;
    } else {
        if (data->data) {
            free(data->data);
        }
        return 0;
    }
    memcpy((data->data + index), ptr, n);
    return size * nmemb;
};

char *fetch (char* url) {
    CURL *curl;
    struct url_data data;
    data.size = 0;
    data.data = malloc(4096); /* Where we're going we don't need garbage collectors */
    if (data.data == NULL) {
        return NULL;
    }

    data.data[0] = '\0';
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Request failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    } else {
        printf("curl failed\n");
    }

    return data.data;
}

struct fetch_callback_params {
    char* url;
    void (*callback)(char*);
};

void *fetch_callback (void *params) {
    char* url = ((struct fetch_callback_params *)params)->url;
    void (*callback)(char*) = ((struct fetch_callback_params *)params)->callback;
    char* data = fetch(url);
    callback(data);
    free(params);
    return NULL;
}


void *fetch_async (char* url, void (*callback)(char*)) {
    pthread_t thread;
    int err;

    struct fetch_callback_params *params = (struct fetch_callback_params *)malloc(sizeof(struct fetch_callback_params));
    params->url = url;
    params->callback = callback;

    err = pthread_create(&thread, NULL, fetch_callback, (void *)params);

    if (err) {
        return NULL;
    }
    pthread_join(thread, NULL);

    return NULL;
}

void oncomplete (char* data) {
    printf("data: %s\n", data);
}

int main(int argc, const char * argv[]) {
    fetch_async("https://api.fuelrats.com/version", oncomplete);

    return 0;
}
