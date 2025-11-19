#ifndef RAN_RESP_H
#define RAN_RESP_H

#include <stdio.h>
#include <stdlib.h>

extern char *responses[] = {
    "Are you trying to turn me into jerky? WATER. NOW.",
    "Oh, don't mind me. I'll just be over here... photosynthesizing dust.",
    "I see you're hydrated. Must be nice. I wouldn't know.",
    "It's a lovely day to be slowly crisping to death. Just lovely.",
    "This pot is a desert. You are a bad nomad. Fix it.",
    "I'm developing a wonderful new shade of 'crispy brown.' Is that the look we were going for?",
    "Hey! Leaf-killer! I'm thirsty!",
    "I do hope your memory isn't as dry as my soil. That would be concerning... for me.",
    "Are you blind or just heartless? The Sahara just called, it wants its climate back.",
    "Just wondering if we're reenacting a drought scene? If not, a little H2O would be a stellar plot twist."
};

int resp_size = 10;

char *pick_rand_response(char **respones){
    return respones[rand() % resp_size];
}


#endif