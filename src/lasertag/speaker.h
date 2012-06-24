#ifndef LASERTAG_SPEAKER_H
#define LASERTAG_SPEAKER_H

/* Initializes the speaker. */
void speaker_init(void);

/* Turns on the speaker and makes it play a tone of the given frequency. */
void speaker_tone(int hz);

/* Turns off the speaker. */
void speaker_off(void);

#endif

