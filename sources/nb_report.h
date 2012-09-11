#ifndef NB_REPORT_H_INCLUDED
#define NB_REPORT_H_INCLUDED

struct nb_report_if {
	const char *name;
	void (*init)(void);
	void (*free)(void);
	void (*report_start)(void);
	void (*report)(void);
	void (*report_final)(void);
};

extern struct nb_report_if nb_reports[];

struct nb_report_if *nb_report_match(const char *name);

#endif
