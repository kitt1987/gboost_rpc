#include "rpc/detail/rpc_package.cc"
#include "unit.test/protocol/rpc_test.pb.h"
#include <gtest/gtest.h>

using namespace gboost_rpc::detail;

class RPCMessageTester : public ::testing::Test {
 public:

  RPCMessageTester()
      : binaries((void*) 0, 0) {

  }

 protected:
  static void SetUpTestCase() {
  }

  static void TearDownTestCase() {
  }

  virtual void SetUp() {
    message.set_text("This is some text. This is some text. This is "
                     "some text. This is some text.");
    descriptor.set_seq(1);
    descriptor.set_service_id(2);
    descriptor.set_method_id(3);
    descriptor.set_message_length(message.ByteSize());
    RPCData message(descriptor, message);
    binaries = message.GetBuffer();
    int8* large = new int8[boost::asio::buffer_size(binaries) * 2];
    large_binaries = boost::asio::mutable_buffer(
        large, boost::asio::buffer_size(binaries) * 2);
    memcpy(large, boost::asio::buffer_cast<int8*>(binaries),
           boost::asio::buffer_size(binaries));
    memcpy(large + boost::asio::buffer_size(binaries),
           boost::asio::buffer_cast<int8*>(binaries),
           boost::asio::buffer_size(binaries));
  }

  virtual void TearDown() {
    int8* buffer_data = boost::asio::buffer_cast<int8*>(binaries);
    delete[] buffer_data;

    buffer_data = boost::asio::buffer_cast<int8*>(large_binaries);
    delete[] buffer_data;
  }

 public:
  TestMessage message;
  RPCMetaData descriptor;
  boost::asio::mutable_buffer binaries;
  boost::asio::mutable_buffer large_binaries;

  bool HasBinaries() {
    return boost::asio::buffer_cast<int8*>(binaries) != NULL;
  }
};

TEST_F(RPCMessageTester, AppendJust4BytesThenTotalDescriptor) {
  ASSERT_TRUE(HasBinaries());
  RPCData rpc_message;
  ASSERT_GT(boost::asio::buffer_size(binaries), 4);
  uint32 handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries), 4);
  ASSERT_EQ(descriptor.ByteSize(), rpc_message.DescriptorLength());
  ASSERT_EQ(handled, 4);
  handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries) + 4, descriptor.ByteSize());
  ASSERT_EQ(handled, descriptor.ByteSize());
  ASSERT_TRUE(rpc_message.Meta() != NULL);
  ASSERT_EQ(rpc_message.Meta()->seq(), descriptor.seq());
  ASSERT_EQ(rpc_message.Meta()->service_id(), descriptor.service_id());
  ASSERT_EQ(rpc_message.Meta()->method_id(), descriptor.method_id());
  ASSERT_EQ(rpc_message.Meta()->message_length(),
            descriptor.message_length());
}

TEST_F(RPCMessageTester, AppendJust4BytesThen5BytesOfDescriptorFollowed10Bytes) {
  ASSERT_TRUE(HasBinaries());
  RPCData rpc_message;
  ASSERT_GT(boost::asio::buffer_size(binaries), 4);
  uint32 handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries), 4);
  ASSERT_EQ(descriptor.ByteSize(), rpc_message.DescriptorLength());
  ASSERT_EQ(handled, 4);
  ASSERT_EQ(15, descriptor.ByteSize());
  handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries) + 4, 10);
  ASSERT_EQ(handled, 10);
  handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries) + 14, 5);
  ASSERT_EQ(handled, 5);
  ASSERT_TRUE(rpc_message.Meta() != NULL);
  ASSERT_EQ(rpc_message.Meta()->seq(), descriptor.seq());
  ASSERT_EQ(rpc_message.Meta()->service_id(), descriptor.service_id());
  ASSERT_EQ(rpc_message.Meta()->method_id(), descriptor.method_id());
  ASSERT_EQ(rpc_message.Meta()->message_length(),
            descriptor.message_length());
}

TEST_F(RPCMessageTester, AppendJust4BytesThen10BytesOfDescriptorFollowed10Bytes) {
  ASSERT_TRUE(HasBinaries());
  RPCData rpc_message;
  ASSERT_GT(boost::asio::buffer_size(binaries), 4);
  uint32 handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries), 4);
  ASSERT_EQ(descriptor.ByteSize(), rpc_message.DescriptorLength());
  ASSERT_EQ(handled, 4);
  ASSERT_EQ(15, descriptor.ByteSize());
  ASSERT_GT(message.ByteSize(), 5);
  handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries) + 4, 10);
  ASSERT_EQ(handled, 10);
  handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries) + 14, 10);
  ASSERT_EQ(handled, 10);
  ASSERT_TRUE(rpc_message.Meta() != NULL);
  ASSERT_EQ(rpc_message.Meta()->seq(), descriptor.seq());
  ASSERT_EQ(rpc_message.Meta()->service_id(), descriptor.service_id());
  ASSERT_EQ(rpc_message.Meta()->method_id(), descriptor.method_id());
  ASSERT_EQ(rpc_message.Meta()->message_length(),
            descriptor.message_length());
}

TEST_F(RPCMessageTester, Append1BytesFollow3Bytes) {
  ASSERT_TRUE(HasBinaries());
  RPCData rpc_message;
  ASSERT_GT(boost::asio::buffer_size(binaries), 4);
  uint32 handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries), 3);
  ASSERT_EQ(handled, 3);
  handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries) + 3, 1);
  ASSERT_EQ(handled, 1);
  ASSERT_EQ(descriptor.ByteSize(), rpc_message.DescriptorLength());
}

TEST_F(RPCMessageTester, Append3BytesFollow2Bytes) {
  ASSERT_TRUE(HasBinaries());
  RPCData rpc_message;
  ASSERT_GT(boost::asio::buffer_size(binaries), 4);
  uint32 handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries), 2);
  ASSERT_EQ(handled, 2);
  handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries) + 2, 3);
  ASSERT_EQ(handled, 3);
  ASSERT_EQ(descriptor.ByteSize(), rpc_message.DescriptorLength());
}

TEST_F(RPCMessageTester, AppendAWholePackage) {
  ASSERT_TRUE(HasBinaries());
  RPCData rpc_message;
  uint32 handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(binaries),
      boost::asio::buffer_size(binaries));
  ASSERT_EQ(descriptor.ByteSize(), rpc_message.DescriptorLength());
  ASSERT_EQ(handled, boost::asio::buffer_size(binaries));
  ASSERT_TRUE(rpc_message.Meta() != NULL);
  ASSERT_EQ(rpc_message.Meta()->seq(), descriptor.seq());
  ASSERT_EQ(rpc_message.Meta()->service_id(), descriptor.service_id());
  ASSERT_EQ(rpc_message.Meta()->method_id(), descriptor.method_id());
  ASSERT_EQ(rpc_message.Meta()->message_length(),
            descriptor.message_length());
  ASSERT_TRUE(rpc_message.Done());
}

TEST_F(RPCMessageTester, AppendMoreThanAWholePackage) {
  ASSERT_TRUE(HasBinaries());
  RPCData rpc_message;
  uint32 handled = rpc_message.AppendBinaries(
      boost::asio::buffer_cast<int8*>(large_binaries),
      boost::asio::buffer_size(large_binaries));
  ASSERT_EQ(descriptor.ByteSize(), rpc_message.DescriptorLength());
  ASSERT_EQ(handled, boost::asio::buffer_size(binaries));
  ASSERT_TRUE(rpc_message.Meta() != NULL);
  ASSERT_EQ(rpc_message.Meta()->seq(), descriptor.seq());
  ASSERT_EQ(rpc_message.Meta()->service_id(), descriptor.service_id());
  ASSERT_EQ(rpc_message.Meta()->method_id(), descriptor.method_id());
  ASSERT_EQ(rpc_message.Meta()->message_length(),
            descriptor.message_length());
  ASSERT_TRUE(rpc_message.Done());
}
