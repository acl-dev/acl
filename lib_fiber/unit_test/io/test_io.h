#pragma once

/* In io_main.cpp */
void io_register(void);

/* In eventfd.cpp */
int test_eventfd(AUT_LINE *test_line, void *arg);

/* In poll.cpp */
int test_poll(AUT_LINE *test_line, void *arg);
