/*
 * This file Copyright (C) 2015 tobbez
 *
 * It may be used under the GNU GPL versions 2 or 3.
 *
 */

#include "event-export-server.h"


#include "transmission.h"
#include "log.h"
#include "torrent.h"
#include "variant.h"

#include <msgpack.h>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>


#include <assert.h>

#define MY_NAME "Event Export Server"

/* event export server socket */
static int ees_sock;
static bool ees_enabled;

char const *default_bind_address = "ipc:///tmp/transmission.ipc";

static bool ees_conf_enabled;
static char ees_conf_bind_addr[1024];

/* Needed for proper save/restore of config file */
bool
tr_eventExportServerGetEnabled(void)
{
        return ees_conf_enabled;
}

char const *
tr_eventExportServerGetBindAddr(void)
{
        return ees_conf_bind_addr;
}


void
tr_eventExportServerInit(tr_variant *settings)
{
        int res;
        char const *bind_addr;

        bool tmp = tr_variantDictFindBool(settings, TR_KEY_event_export_server_enabled, &ees_conf_enabled);
        if (! tmp|| !ees_conf_enabled) {
                /* Disabled */
                tr_logAddNamedInfo(MY_NAME, "DISABLED %i %i", tmp, ees_conf_enabled);
                ees_enabled = false;
                return;
        }
        ees_enabled = true;

        ees_sock = nn_socket(AF_SP, NN_PUB);
        if (ees_sock < 0) {
                tr_logAddNamedError(MY_NAME, "nn_socket failed: %s", nn_strerror(errno));
                return;
        }

        if (!tr_variantDictFindStr(settings, TR_KEY_event_export_server_bind_addr, &bind_addr, NULL)) {
                tr_strlcpy(ees_conf_bind_addr, default_bind_address, 1024);
        } else {
                tr_strlcpy(ees_conf_bind_addr, bind_addr, 1024);
        }

        res = nn_bind(ees_sock, ees_conf_bind_addr);
        if (res < 0) {
                tr_logAddNamedError(MY_NAME, "nn_bind failed: %s", nn_strerror(errno));
                ees_enabled = false;
        } else {
                tr_logAddNamedInfo(MY_NAME, "Listening on %s", ees_conf_bind_addr);
        }
}

/* Pack a full string.
 * This allows to use one function call to pack a string, rather than having
 * to use two separate ones.
 */
static void
msgpack_pack_fstr(msgpack_packer *pk, char const *str, size_t l)
{
        msgpack_pack_str(pk, l);
        msgpack_pack_str_body(pk, str, l);
}

/*
 * Convenience function for packing a C string (NULL terminated)
 */
static void
msgpack_pack_fcstr(msgpack_packer *pk, char const *str)
{
        msgpack_pack_fstr(pk, str, strlen(str));
}

static void
msgpack_pack_bool(msgpack_packer *pk, bool b)
{
        if (b) {
                msgpack_pack_true(pk);
        } else {
                msgpack_pack_false(pk);
        }
}

/*
 * Add a packet header, consisting of a map containing:
 * - the torent info hash
 * - the name of the data field to be sent
 * - the key for the packet map's data member
 */
static void
add_packet_header(msgpack_packer *pk, tr_torrent *tor, char const *field_name)
{
        msgpack_pack_map(pk, 3);

        msgpack_pack_fstr(pk, "torrent", strlen("torrent"));
        msgpack_pack_fstr(pk, tor->info.hashString, strlen(tor->info.hashString));

        msgpack_pack_fstr(pk, "type", strlen("type"));
        msgpack_pack_str_body(pk, field_name, strlen(field_name));

        msgpack_pack_fstr(pk, "data", strlen("data"));
}


/* The packet_builder was introduced to reduce duplication in the
 * tr_eventExportServerSend* functions.
 */
typedef struct packet_builder {
        msgpack_sbuffer sbuf;
        msgpack_packer pk;
} packet_builder;

static void
packet_builder_init(packet_builder *pb, tr_torrent *tor, char const *type)
{
        msgpack_sbuffer_init(&pb->sbuf);
        msgpack_packer_init(&pb->pk, &pb->sbuf, msgpack_sbuffer_write);
        add_packet_header(&pb->pk, tor, type);
}

static void
packet_builder_send(packet_builder *pb)
{
        /*
         * FIXME:
         * This is pretty ineffective, since we do all the work even if the
         * packet isn't supposed to be sent. Then again, you will likely not
         * be using this patch unless you plan to enable its functionality.
         */
        if (ees_enabled) {
                nn_send(ees_sock, pb->sbuf.data, pb->sbuf.size, 0);
        }
        msgpack_sbuffer_destroy(&pb->sbuf);
}


void
tr_eventExportServerSendActivityDate(tr_torrent *tor, time_t activityDate)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "activityDate");
        msgpack_pack_int64(&pb.pk, activityDate);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendBandwidthPriority(tr_torrent *tor, tr_priority_t priority)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "bandwidthPriority");
        msgpack_pack_int8(&pb.pk, priority);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendCorruptEver(tr_torrent *tor, uint64_t corruptEver)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "corruptEver");
        msgpack_pack_uint64(&pb.pk, corruptEver);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendDoneDate(tr_torrent *tor, time_t doneDate)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "doneDate");
        msgpack_pack_int64(&pb.pk, doneDate);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendDownloadDir(tr_torrent *tor, char *downloadDir)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "downloadDir");
        msgpack_pack_fstr(&pb.pk, downloadDir, strlen(downloadDir));
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendDownloadedEver(tr_torrent *tor, uint64_t downloadedEver)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "downloadedEver");
        msgpack_pack_uint64(&pb.pk, downloadedEver);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendDownloadLimit(tr_torrent *tor, uint32_t downloadLimit)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "downloadLimit");
        msgpack_pack_uint32(&pb.pk, downloadLimit);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendDownloadLimited(tr_torrent *tor, bool downloadLimited)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "downloadLimited");
        msgpack_pack_bool(&pb.pk, downloadLimited);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendError(tr_torrent *tor, int8_t error)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "error");
        msgpack_pack_int(&pb.pk, error);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendErrorString(tr_torrent *tor, char *errorString)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "errorString");
        msgpack_pack_fstr(&pb.pk, errorString, strlen(errorString));
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendHonorsSessionLimits(tr_torrent *tor, bool honorsSessionLimits)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "honorsSessionLimits");
        msgpack_pack_bool(&pb.pk, honorsSessionLimits);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendMaxConnectedPeers(tr_torrent *tor, uint16_t maxConnectedPeers)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "maxConnectedPeers");
        msgpack_pack_uint16(&pb.pk, maxConnectedPeers);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendPeersConnected(tr_torrent *tor, int32_t peersConnected)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "peersConnected");
        msgpack_pack_int32(&pb.pk, peersConnected);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendPeersGettingFromUs(tr_torrent *tor, int32_t peersGettingFromUs)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "peersGettingFromUs");
        msgpack_pack_int32(&pb.pk, peersGettingFromUs);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendPeersSendingToUs(tr_torrent *tor, int32_t peersSendingToUs)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "peersSendingToUs");
        msgpack_pack_int32(&pb.pk, peersSendingToUs);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendQueuePosition(tr_torrent *tor, int32_t queuePosition)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "queuePosition");
        msgpack_pack_int32(&pb.pk, queuePosition);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendSecondsDownloading(tr_torrent *tor, int32_t secondsDownloading)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "secondsDownloading");
        msgpack_pack_int32(&pb.pk, secondsDownloading);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendSecondsSeeding(tr_torrent *tor, int32_t secondsSeeding)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "secondsSeeding");
        msgpack_pack_int32(&pb.pk, secondsSeeding);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendSeedIdleLimit(tr_torrent *tor, uint16_t seedIdleLimit)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "seedIdleLimit");
        msgpack_pack_uint16(&pb.pk, seedIdleLimit);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendSeedIdleMode(tr_torrent *tor, int32_t seedIdleMode)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "seedIdleMode");
        msgpack_pack_int32(&pb.pk, seedIdleMode);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendSeedRatioLimit(tr_torrent *tor, double seedRatioLimit)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "seedRatioLimit");
        msgpack_pack_double(&pb.pk, seedRatioLimit);
        packet_builder_send(&pb);
}

void tr_eventExportServerSendSeedRatioMode(tr_torrent *tor, int32_t seedRatioMode)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "seedRatioMode");
        msgpack_pack_int32(&pb.pk, seedRatioMode);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendAnnounceDone(
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
                int32_t tier)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "trackerStat");

        msgpack_pack_map(&pb.pk, 10);

        msgpack_pack_fcstr(&pb.pk, "announce");
        msgpack_pack_fcstr(&pb.pk, announce);

        msgpack_pack_fcstr(&pb.pk, "host");
        msgpack_pack_fcstr(&pb.pk, host);

        msgpack_pack_fcstr(&pb.pk, "id");
        msgpack_pack_uint32(&pb.pk, id);

        msgpack_pack_fcstr(&pb.pk, "lastAnnouncePeerCount");
        msgpack_pack_int32(&pb.pk, lastAnnouncePeerCount);

        msgpack_pack_fcstr(&pb.pk, "lastAnnounceResult");
        msgpack_pack_fcstr(&pb.pk, lastAnnounceResult);

        msgpack_pack_fcstr(&pb.pk, "lastAnnounceSucceeded");
        msgpack_pack_bool(&pb.pk, lastAnnounceSucceeded);

        msgpack_pack_fcstr(&pb.pk, "lastAnnounceTime");
        msgpack_pack_int64(&pb.pk, lastAnnounceTime);

        msgpack_pack_fcstr(&pb.pk, "lastAnnounceTimedOut");
        msgpack_pack_bool(&pb.pk, lastAnnounceTimedOut);

        msgpack_pack_fcstr(&pb.pk, "nextAnnounceTime");
        msgpack_pack_int64(&pb.pk, nextAnnounceTime);

        msgpack_pack_fcstr(&pb.pk, "tier");
        msgpack_pack_int32(&pb.pk, tier);

        packet_builder_send(&pb);
}

void
tr_eventExportServerSendScrapeDone(
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
                int32_t tier)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "trackerStat");

        msgpack_pack_map(&pb.pk, 12);

        msgpack_pack_fcstr(&pb.pk, "downloadCount");
        msgpack_pack_int32(&pb.pk, downloadCount);

        msgpack_pack_fcstr(&pb.pk, "host");
        msgpack_pack_fcstr(&pb.pk, host);

        msgpack_pack_fcstr(&pb.pk, "id");
        msgpack_pack_uint32(&pb.pk, id);

        msgpack_pack_fcstr(&pb.pk, "lastScrapeResult");
        msgpack_pack_fcstr(&pb.pk, lastScrapeResult);

        msgpack_pack_fcstr(&pb.pk, "lastScrapeSucceeded");
        msgpack_pack_bool(&pb.pk, lastScrapeSucceeded);

        msgpack_pack_fcstr(&pb.pk, "lastScrapeTime");
        msgpack_pack_int64(&pb.pk, lastScrapeTime);

        msgpack_pack_fcstr(&pb.pk, "lastScrapeTimedOut");
        msgpack_pack_bool(&pb.pk, lastScrapeTimedOut);

        msgpack_pack_fcstr(&pb.pk, "leecherCount");
        msgpack_pack_int32(&pb.pk, leecherCount);

        msgpack_pack_fcstr(&pb.pk, "nextScrapeTime");
        msgpack_pack_int64(&pb.pk, nextScrapeTime);

        msgpack_pack_fcstr(&pb.pk, "scrape");
        msgpack_pack_fcstr(&pb.pk, scrape);

        msgpack_pack_fcstr(&pb.pk, "seederCount");
        msgpack_pack_int32(&pb.pk, seederCount);

        msgpack_pack_fcstr(&pb.pk, "tier");
        msgpack_pack_int32(&pb.pk, tier);

        packet_builder_send(&pb);
}

void
tr_eventExportServerSendUploadedEver(tr_torrent *tor, uint64_t uploadedEver)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "uploadedEver");
        msgpack_pack_uint64(&pb.pk, uploadedEver);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendUploadLimit(tr_torrent *tor, uint32_t uploadLimit)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "uploadLimit");
        msgpack_pack_uint32(&pb.pk, uploadLimit);
        packet_builder_send(&pb);
}

void
tr_eventExportServerSendUploadLimited(tr_torrent *tor, bool uploadLimited)
{
        packet_builder pb;
        packet_builder_init(&pb, tor, "uploadLimited");
        msgpack_pack_bool(&pb.pk, uploadLimited);
        packet_builder_send(&pb);
}
