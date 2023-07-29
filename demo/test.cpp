std::shared_ptr<vsomeip::application> mDmSomeipApp;

mDmSomeipApp(vsomeip::runtime::get()->create_application(DM_VSOMEIP_APP, VLAN04_CONFIG_FILE))

    if (!mDmSomeipApp->init())
{
    LOGGER->Nhh_Info("[DM_M] Couldn't initialize application");
    exit(1);
}

auto self = shared_from_this();
using namespace std::placeholders;

mDmSomeipApp->register_state_handler([self, this](vsomeip::state_type_e state)
                                     { onState(state); });

for (const auto &[service, instance, method] : mMsgHandleSet)
{
    mDmSomeipApp->register_message_handler(service, instance, method, std::bind(&DmManager::onMessage, this, _1));
}

mDmSomeipApp->register_send_request_message_session_handler([this](vsomeip::session_t in, vsomeip::session_t out)
                                                            { onReqSessionCb(in, out); });

for (const auto &iter : mAvailableSet)
{
    mDmSomeipApp->register_availability_handler(std::get<0>(iter), std::get<1>(iter),
                                                std::bind(&DmManager::onAvailabillity, this, _1, _2, _3), std::get<2>(iter), std::get<3>(iter));
}

std::thread th([this]
               { 
            LOGGER->Nhh_Warn("[DM_M] DmManager mDmSomeipApp start");
            mDmSomeipApp->start(); });
th.detach();

void DmManager::onState(vsomeip::state_type_e state)
{
    bool isRegistered = false;
    if (state == vsomeip::state_type_e::ST_REGISTERED)
    {
        isRegistered = true;
    }

    LOGGER->Nhh_Warn("[DM_M] onState called, application:%s, isRegistered:%d",
                     mDmSomeipApp->get_name().c_str(), isRegistered);
    if (isRegistered)
    {
        std::unique_lock<std::mutex> lock(mSomeipMutex);
        for (const auto &iter : mAvailableSet)
        {
            mDmSomeipApp->request_service(std::get<0>(iter), std::get<1>(iter), std::get<2>(iter), std::get<3>(iter));
        }
    }
}

void DmManager::onMessage(const std::shared_ptr<vsomeip::message> &msg)
{
#if 0
        std::stringstream its_message;
        its_message << "onMessage called, ["
                    << std::setw(4) << std::setfill('0') << std::hex
                    << msg->get_service() << "."
                    << std::setw(4) << std::setfill('0') << std::hex
                    << msg->get_instance() << "."
                    << std::setw(4) << std::setfill('0') << std::hex
                    << msg->get_method() << "]["
                    << std::setw(4) << std::setfill('0') << std::hex
                    << msg->get_client() << "/"
                    << std::setw(4) << std::setfill('0') << std::hex
                    << msg->get_session()
                    << "] = ";

        std::shared_ptr<vsomeip::payload> its_payload = msg->get_payload();
        its_message << "(" << std::dec << its_payload->get_length() << ") ";
        for (uint32_t i = 0; i < its_payload->get_length(); ++i)
            its_message << std::hex << std::setw(2) << std::setfill('0') << (int)its_payload->get_data()[i] << " ";
        LOGGER->Nhh_Warn("[DM_M] %s", its_message.str().c_str());
#endif
}

void DmManager::onReqSessionCb(vsomeip::session_t in, vsomeip::session_t out)
{
}

void DmManager::onAvailabillity(vsomeip::service_t service, vsomeip::instance_t instance, bool isAvailable)
{
    LOGGER->Nhh_Warn("[DM_M] onAvailabillity called, service:0x%04x, instance:%d, isAvailable:%d",
                     service, instance, isAvailable);
    std::unique_lock<std::mutex> lock(mSomeipMutex);
    mServiceAvailableMap[service] = {instance, isAvailable};
}
