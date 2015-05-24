/*
 * This file Copyright (C) 2015 tobbez
 *
 * It may be used under the GNU GPL versions 2 or 3.
 *
 */

#ifndef __TRANSMISSION__
 #error only libtransmission should #include this header.
#endif

#ifndef TR_EVENT_EXPORT_SERVER_H
#define TR_EVENT_EXPORT_SERVER_H

#include "transmission.h"
#include "variant.h"

void tr_eventExportServerInit(tr_variant *settings);
bool tr_eventExportServerGetEnabled(void);
char const * tr_eventExportServerGetBindAddr(void);

void tr_eventExportServerSendActivityDate(tr_torrent *tor, time_t activityDate);
void tr_eventExportServerSendBandwidthPriority(tr_torrent *tor, tr_priority_t priority);
void tr_eventExportServerSendCorruptEver(tr_torrent *tor, uint64_t corruptEver);
void tr_eventExportServerSendDoneDate(tr_torrent *tor, time_t doneDate);
void tr_eventExportServerSendDownloadDir(tr_torrent *tor, char *downloadDir);
void tr_eventExportServerSendDownloadedEver(tr_torrent *tor, uint64_t downloadedEver);
void tr_eventExportServerSendDownloadLimit(tr_torrent *tor, uint32_t downloadLimit);
void tr_eventExportServerSendDownloadLimited(tr_torrent *tor, bool downloadLimited);
void tr_eventExportServerSendError(tr_torrent *tor, int8_t error);
void tr_eventExportServerSendErrorString(tr_torrent *tor, char *errorString);
void tr_eventExportServerSendHonorsSessionLimits(tr_torrent *tor, bool honorsSessionLimits);
void tr_eventExportServerSendMaxConnectedPeers(tr_torrent *tor, uint16_t maxConnectedPeers);
void tr_eventExportServerSendPeersConnected(tr_torrent *tor, int32_t peersConnected);
void tr_eventExportServerSendPeersGettingFromUs(tr_torrent *tor, int32_t peersGettingFromUs);
void tr_eventExportServerSendPeersSendingToUs(tr_torrent *tor, int32_t peersSendingToUs);
void tr_eventExportServerSendQueuePosition(tr_torrent *tor, int32_t queuePosition);
void tr_eventExportServerSendSecondsDownloading(tr_torrent *tor, int32_t secondsDownloading);
void tr_eventExportServerSendSecondsSeeding(tr_torrent *tor, int32_t secondsSeeding);
void tr_eventExportServerSendSeedIdleLimit(tr_torrent *tor, uint16_t seedIdleLimit);
void tr_eventExportServerSendSeedIdleMode(tr_torrent *tor, int32_t seedIdleMode);
void tr_eventExportServerSendSeedRatioLimit(tr_torrent *tor, double seedRatioLimit);
void tr_eventExportServerSendSeedRatioMode(tr_torrent *tor, int32_t seedRatioMode);

/* It would have been easier to pass in a tr_tier, but it's private to
 * announcer.c, and it's too much work to extract tr_tier (and its dependencies)
 * into announcer.h.
 */
void tr_eventExportServerSendAnnounceDone(
                tr_torrent *tor,
                char *announce,
                char *host,
                uint32_t id,
                int32_t lastAnnouncePeerCount,
                char *lastAnnounceResult,
                bool lastAnnounceSucceeded,
                time_t lastAnnounceTime,
                bool lastAnnounceTimedOut,
                time_t nextAnnounceTime,
                int32_t tier);

void tr_eventExportServerSendScrapeDone(
                tr_torrent *tor,
                int32_t downloadCount,
                char *host,
                uint32_t id,
                char *lastScrapeResult,
                bool lastScrapeSucceeded,
                time_t lastScrapeTime,
                bool lastScrapeTimedOut,
                int32_t leecherCount,
                time_t nextScrapeTime,
                char *scrape,
                int32_t seederCount,
                int32_t tier);

void tr_eventExportServerSendUploadedEver(tr_torrent *tor, uint64_t uploadedEver);
void tr_eventExportServerSendUploadLimit(tr_torrent *tor, uint32_t uploadLimit);
void tr_eventExportServerSendUploadLimited(tr_torrent *tor, bool uploadLimited);

#endif
