#pragma once

/* In tbox_main.cpp */
void tbox_register(void);

/* In tbox_wait.cpp */
int tbox_thread_wait(AUT_LINE *test_line, void *arg);
int tbox_fiber_wait(AUT_LINE *test_line, void *arg);

/* In tbox_fiber.cpp */
int tbox_fiber_consume(AUT_LINE *test_line, void *arg);

/* In tbox_mixed.cpp */
int tbox_mixed_consume(AUT_LINE *test_line, void *arg);
