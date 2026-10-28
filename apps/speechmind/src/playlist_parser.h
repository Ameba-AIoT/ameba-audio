#ifndef AMEBA_AUDIO_APPS_SPEECHMIND_PLAYLIST_PARSER_H
#define AMEBA_AUDIO_APPS_SPEECHMIND_PLAYLIST_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PlaylistParser PlaylistParser;

PlaylistParser *PlaylistParser_Create(void);
void PlaylistParser_Destroy(PlaylistParser *parser);
void PlaylistParser_WakeUp(PlaylistParser *parser);
void PlaylistParser_VadTimeout(PlaylistParser *parser);
void PlaylistParser_AsrMsg(PlaylistParser *parser, const char *json);

#ifdef __cplusplus
}
#endif

#endif /* AMEBA_AUDIO_APPS_SPEECHMIND_PLAYLIST_PARSER_H */
