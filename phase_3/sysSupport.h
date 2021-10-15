#ifndef SYSSUPPORT_H
#define SYSSUPPORT_H

void supexHandler();
void sysHandler(support_t* except_supp, state_t* exc_state, unsigned int sysNum);

void terminate();
void get_tod(state_t* exc_state);
void write_printer(support_t* except_supp, state_t* exc_state);
void write_terminal(support_t* except_supp, state_t* exc_state);
void read_terminal(support_t* except_supp, state_t* exc_state);


#endif