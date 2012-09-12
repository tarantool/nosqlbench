#ifndef NB_CONFIG_H_INCLUDED
#define NB_CONFIG_H_INCLUDED

#define NB_DEFAULT_CONFIG "nosqlbench.conf"

int nb_config_parse(struct nb_options *opts, char *file);

#endif
