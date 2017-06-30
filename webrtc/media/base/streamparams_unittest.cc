/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/media/base/streamparams.h"
#include "webrtc/media/base/testutils.h"
#include "webrtc/rtc_base/arraysize.h"
#include "webrtc/rtc_base/gunit.h"

static const uint32_t kSsrcs1[] = {1};
static const uint32_t kSsrcs2[] = {1, 2};
static const uint32_t kSsrcs3[] = {1, 2, 3};
static const uint32_t kRtxSsrcs3[] = {4, 5, 6};

static cricket::StreamParams CreateStreamParamsWithSsrcGroup(
    const std::string& semantics,
    const uint32_t ssrcs_in[],
    size_t len) {
  cricket::StreamParams stream;
  std::vector<uint32_t> ssrcs(ssrcs_in, ssrcs_in + len);
  cricket::SsrcGroup sg(semantics, ssrcs);
  stream.ssrcs = ssrcs;
  stream.ssrc_groups.push_back(sg);
  return stream;
}

TEST(SsrcGroup, EqualNotEqual) {
  cricket::SsrcGroup ssrc_groups[] = {
    cricket::SsrcGroup("ABC", MAKE_VECTOR(kSsrcs1)),
    cricket::SsrcGroup("ABC", MAKE_VECTOR(kSsrcs2)),
    cricket::SsrcGroup("Abc", MAKE_VECTOR(kSsrcs2)),
    cricket::SsrcGroup("abc", MAKE_VECTOR(kSsrcs2)),
  };

  for (size_t i = 0; i < arraysize(ssrc_groups); ++i) {
    for (size_t j = 0; j < arraysize(ssrc_groups); ++j) {
      EXPECT_EQ((ssrc_groups[i] == ssrc_groups[j]), (i == j));
      EXPECT_EQ((ssrc_groups[i] != ssrc_groups[j]), (i != j));
    }
  }
}

TEST(SsrcGroup, HasSemantics) {
  cricket::SsrcGroup sg1("ABC", MAKE_VECTOR(kSsrcs1));
  EXPECT_TRUE(sg1.has_semantics("ABC"));

  cricket::SsrcGroup sg2("Abc", MAKE_VECTOR(kSsrcs1));
  EXPECT_FALSE(sg2.has_semantics("ABC"));

  cricket::SsrcGroup sg3("abc", MAKE_VECTOR(kSsrcs1));
  EXPECT_FALSE(sg3.has_semantics("ABC"));
}

TEST(SsrcGroup, ToString) {
  cricket::SsrcGroup sg1("ABC", MAKE_VECTOR(kSsrcs1));
  EXPECT_STREQ("{semantics:ABC;ssrcs:[1]}", sg1.ToString().c_str());
}

TEST(StreamParams, CreateLegacy) {
  const uint32_t ssrc = 7;
  cricket::StreamParams one_sp = cricket::StreamParams::CreateLegacy(ssrc);
  EXPECT_EQ(1U, one_sp.ssrcs.size());
  EXPECT_EQ(ssrc, one_sp.first_ssrc());
  EXPECT_TRUE(one_sp.has_ssrcs());
  EXPECT_TRUE(one_sp.has_ssrc(ssrc));
  EXPECT_FALSE(one_sp.has_ssrc(ssrc+1));
  EXPECT_FALSE(one_sp.has_ssrc_groups());
  EXPECT_EQ(0U, one_sp.ssrc_groups.size());
}

TEST(StreamParams, HasSsrcGroup) {
  cricket::StreamParams sp =
      CreateStreamParamsWithSsrcGroup("XYZ", kSsrcs2, arraysize(kSsrcs2));
  EXPECT_EQ(2U, sp.ssrcs.size());
  EXPECT_EQ(kSsrcs2[0], sp.first_ssrc());
  EXPECT_TRUE(sp.has_ssrcs());
  EXPECT_TRUE(sp.has_ssrc(kSsrcs2[0]));
  EXPECT_TRUE(sp.has_ssrc(kSsrcs2[1]));
  EXPECT_TRUE(sp.has_ssrc_group("XYZ"));
  EXPECT_EQ(1U, sp.ssrc_groups.size());
  EXPECT_EQ(2U, sp.ssrc_groups[0].ssrcs.size());
  EXPECT_EQ(kSsrcs2[0], sp.ssrc_groups[0].ssrcs[0]);
  EXPECT_EQ(kSsrcs2[1], sp.ssrc_groups[0].ssrcs[1]);
}

TEST(StreamParams, GetSsrcGroup) {
  cricket::StreamParams sp =
      CreateStreamParamsWithSsrcGroup("XYZ", kSsrcs2, arraysize(kSsrcs2));
  EXPECT_EQ(NULL, sp.get_ssrc_group("xyz"));
  EXPECT_EQ(&sp.ssrc_groups[0], sp.get_ssrc_group("XYZ"));
}

TEST(StreamParams, EqualNotEqual) {
  cricket::StreamParams l1 = cricket::StreamParams::CreateLegacy(1);
  cricket::StreamParams l2 = cricket::StreamParams::CreateLegacy(2);
  cricket::StreamParams sg1 =
      CreateStreamParamsWithSsrcGroup("ABC", kSsrcs1, arraysize(kSsrcs1));
  cricket::StreamParams sg2 =
      CreateStreamParamsWithSsrcGroup("ABC", kSsrcs2, arraysize(kSsrcs2));
  cricket::StreamParams sg3 =
      CreateStreamParamsWithSsrcGroup("Abc", kSsrcs2, arraysize(kSsrcs2));
  cricket::StreamParams sg4 =
      CreateStreamParamsWithSsrcGroup("abc", kSsrcs2, arraysize(kSsrcs2));
  cricket::StreamParams sps[] = {l1, l2, sg1, sg2, sg3, sg4};

  for (size_t i = 0; i < arraysize(sps); ++i) {
    for (size_t j = 0; j < arraysize(sps); ++j) {
      EXPECT_EQ((sps[i] == sps[j]), (i == j));
      EXPECT_EQ((sps[i] != sps[j]), (i != j));
    }
  }
}

TEST(StreamParams, FidFunctions) {
  uint32_t fid_ssrc;

  cricket::StreamParams sp = cricket::StreamParams::CreateLegacy(1);
  EXPECT_FALSE(sp.AddFidSsrc(10, 20));
  EXPECT_TRUE(sp.AddFidSsrc(1, 2));
  EXPECT_TRUE(sp.GetFidSsrc(1, &fid_ssrc));
  EXPECT_EQ(2u, fid_ssrc);
  EXPECT_FALSE(sp.GetFidSsrc(15, &fid_ssrc));

  sp.add_ssrc(20);
  EXPECT_TRUE(sp.AddFidSsrc(20, 30));
  EXPECT_TRUE(sp.GetFidSsrc(20, &fid_ssrc));
  EXPECT_EQ(30u, fid_ssrc);

  // Manually create SsrcGroup to test bounds-checking
  // in GetSecondarySsrc. We construct an invalid StreamParams
  // for this.
  std::vector<uint32_t> fid_vector;
  fid_vector.push_back(13);
  cricket::SsrcGroup invalid_fid_group(cricket::kFidSsrcGroupSemantics,
                                        fid_vector);
  cricket::StreamParams sp_invalid;
  sp_invalid.add_ssrc(13);
  sp_invalid.ssrc_groups.push_back(invalid_fid_group);
  EXPECT_FALSE(sp_invalid.GetFidSsrc(13, &fid_ssrc));
}

TEST(StreamParams, GetPrimaryAndFidSsrcs) {
  cricket::StreamParams sp;
  sp.ssrcs.push_back(1);
  sp.ssrcs.push_back(2);
  sp.ssrcs.push_back(3);

  std::vector<uint32_t> primary_ssrcs;
  sp.GetPrimarySsrcs(&primary_ssrcs);
  std::vector<uint32_t> fid_ssrcs;
  sp.GetFidSsrcs(primary_ssrcs, &fid_ssrcs);
  ASSERT_EQ(1u, primary_ssrcs.size());
  EXPECT_EQ(1u, primary_ssrcs[0]);
  ASSERT_EQ(0u, fid_ssrcs.size());

  sp.ssrc_groups.push_back(
      cricket::SsrcGroup(cricket::kSimSsrcGroupSemantics, sp.ssrcs));
  sp.AddFidSsrc(1, 10);
  sp.AddFidSsrc(2, 20);

  primary_ssrcs.clear();
  sp.GetPrimarySsrcs(&primary_ssrcs);
  fid_ssrcs.clear();
  sp.GetFidSsrcs(primary_ssrcs, &fid_ssrcs);
  ASSERT_EQ(3u, primary_ssrcs.size());
  EXPECT_EQ(1u, primary_ssrcs[0]);
  EXPECT_EQ(2u, primary_ssrcs[1]);
  EXPECT_EQ(3u, primary_ssrcs[2]);
  ASSERT_EQ(2u, fid_ssrcs.size());
  EXPECT_EQ(10u, fid_ssrcs[0]);
  EXPECT_EQ(20u, fid_ssrcs[1]);
}

TEST(StreamParams, FecFrFunctions) {
  uint32_t fecfr_ssrc;

  cricket::StreamParams sp = cricket::StreamParams::CreateLegacy(1);
  EXPECT_FALSE(sp.AddFecFrSsrc(10, 20));
  EXPECT_TRUE(sp.AddFecFrSsrc(1, 2));
  EXPECT_TRUE(sp.GetFecFrSsrc(1, &fecfr_ssrc));
  EXPECT_EQ(2u, fecfr_ssrc);
  EXPECT_FALSE(sp.GetFecFrSsrc(15, &fecfr_ssrc));

  sp.add_ssrc(20);
  EXPECT_TRUE(sp.AddFecFrSsrc(20, 30));
  EXPECT_TRUE(sp.GetFecFrSsrc(20, &fecfr_ssrc));
  EXPECT_EQ(30u, fecfr_ssrc);

  // Manually create SsrcGroup to test bounds-checking
  // in GetSecondarySsrc. We construct an invalid StreamParams
  // for this.
  std::vector<uint32_t> fecfr_vector;
  fecfr_vector.push_back(13);
  cricket::SsrcGroup invalid_fecfr_group(cricket::kFecFrSsrcGroupSemantics,
                                         fecfr_vector);
  cricket::StreamParams sp_invalid;
  sp_invalid.add_ssrc(13);
  sp_invalid.ssrc_groups.push_back(invalid_fecfr_group);
  EXPECT_FALSE(sp_invalid.GetFecFrSsrc(13, &fecfr_ssrc));
}

TEST(StreamParams, ToString) {
  cricket::StreamParams sp =
      CreateStreamParamsWithSsrcGroup("XYZ", kSsrcs2, arraysize(kSsrcs2));
  EXPECT_STREQ("{ssrcs:[1,2];ssrc_groups:{semantics:XYZ;ssrcs:[1,2]};}",
               sp.ToString().c_str());
}

TEST(StreamParams, TestIsOneSsrcStream_LegacyStream) {
  EXPECT_TRUE(
      cricket::IsOneSsrcStream(cricket::StreamParams::CreateLegacy(13)));
}

TEST(StreamParams, TestIsOneSsrcStream_SingleRtxStream) {
  cricket::StreamParams stream;
  stream.add_ssrc(13);
  EXPECT_TRUE(stream.AddFidSsrc(13, 14));
  EXPECT_TRUE(cricket::IsOneSsrcStream(stream));
}

TEST(StreamParams, TestIsOneSsrcStream_SingleFlexfecStream) {
  cricket::StreamParams stream;
  stream.add_ssrc(13);
  EXPECT_TRUE(stream.AddFecFrSsrc(13, 14));
  EXPECT_TRUE(cricket::IsOneSsrcStream(stream));
}

TEST(StreamParams, TestIsOneSsrcStream_SingleFlexfecAndRtxStream) {
  cricket::StreamParams stream;
  stream.add_ssrc(13);
  EXPECT_TRUE(stream.AddFecFrSsrc(13, 14));
  EXPECT_TRUE(stream.AddFidSsrc(13, 15));
  EXPECT_TRUE(cricket::IsOneSsrcStream(stream));
}

TEST(StreamParams, TestIsOneSsrcStream_SimulcastStream) {
  EXPECT_FALSE(cricket::IsOneSsrcStream(
      cricket::CreateSimStreamParams("cname", MAKE_VECTOR(kSsrcs2))));
  EXPECT_FALSE(cricket::IsOneSsrcStream(
      cricket::CreateSimStreamParams("cname", MAKE_VECTOR(kSsrcs3))));
}

TEST(StreamParams, TestIsOneSsrcStream_SimRtxStream) {
  cricket::StreamParams stream =
      cricket::CreateSimWithRtxStreamParams("cname",
                                            MAKE_VECTOR(kSsrcs3),
                                            MAKE_VECTOR(kRtxSsrcs3));
  EXPECT_FALSE(cricket::IsOneSsrcStream(stream));
}

TEST(StreamParams, TestIsSimulcastStream_LegacyStream) {
  EXPECT_FALSE(
      cricket::IsSimulcastStream(cricket::StreamParams::CreateLegacy(13)));
}

TEST(StreamParams, TestIsSimulcastStream_SingleRtxStream) {
  cricket::StreamParams stream;
  stream.add_ssrc(13);
  EXPECT_TRUE(stream.AddFidSsrc(13, 14));
  EXPECT_FALSE(cricket::IsSimulcastStream(stream));
}

TEST(StreamParams, TestIsSimulcastStream_SimulcastStream) {
  EXPECT_TRUE(cricket::IsSimulcastStream(
      cricket::CreateSimStreamParams("cname", MAKE_VECTOR(kSsrcs2))));
  EXPECT_TRUE(cricket::IsSimulcastStream(
      cricket::CreateSimStreamParams("cname", MAKE_VECTOR(kSsrcs3))));
}

TEST(StreamParams, TestIsSimulcastStream_SimRtxStream) {
  cricket::StreamParams stream =
      cricket::CreateSimWithRtxStreamParams("cname",
                                            MAKE_VECTOR(kSsrcs3),
                                            MAKE_VECTOR(kRtxSsrcs3));
  EXPECT_TRUE(cricket::IsSimulcastStream(stream));
}

TEST(StreamParams, TestIsSimulcastStream_InvalidStreams) {
  // stream1 has extra non-sim, non-fid ssrc.
  cricket::StreamParams stream1 =
      cricket::CreateSimWithRtxStreamParams("cname",
                                            MAKE_VECTOR(kSsrcs3),
                                            MAKE_VECTOR(kRtxSsrcs3));
  stream1.add_ssrc(25);
  EXPECT_FALSE(cricket::IsSimulcastStream(stream1));

  // stream2 has invalid fid-group (no primary).
  cricket::StreamParams stream2;
  stream2.add_ssrc(13);
  EXPECT_TRUE(stream2.AddFidSsrc(13, 14));
  std::remove(stream2.ssrcs.begin(), stream2.ssrcs.end(), 13u);
  EXPECT_FALSE(cricket::IsSimulcastStream(stream2));

  // stream3 has two SIM groups.
  cricket::StreamParams stream3 =
      cricket::CreateSimStreamParams("cname", MAKE_VECTOR(kSsrcs2));
  std::vector<uint32_t> sim_ssrcs = MAKE_VECTOR(kRtxSsrcs3);
  cricket::SsrcGroup sg(cricket::kSimSsrcGroupSemantics, sim_ssrcs);
  for (size_t i = 0; i < sim_ssrcs.size(); i++) {
    stream3.add_ssrc(sim_ssrcs[i]);
  }
  stream3.ssrc_groups.push_back(sg);
  EXPECT_FALSE(cricket::IsSimulcastStream(stream3));
}
