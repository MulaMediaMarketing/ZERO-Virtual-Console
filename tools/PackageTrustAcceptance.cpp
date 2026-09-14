#include "PackageTrust.h"
#include <windows.h>
#include <bcrypt.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
std::filesystem::path makeRoot() {
    wchar_t temp[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, temp)) return {};
    return std::filesystem::path(temp) / (L"zero-package-trust-" + std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()));
}
bool writeText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc); if (!f) return false; f << text; f.flush(); return f.good();
}
void check(bool value, const char* name, bool& all) { std::cout << (value ? "[PASS] " : "[FAIL] ") << name << "\n"; all &= value; }
std::string hex(const unsigned char* p, size_t n) { std::ostringstream s; s << std::hex << std::setfill('0'); for(size_t i=0;i<n;++i) s << std::setw(2) << (unsigned)p[i]; return s.str(); }
std::string b64(const unsigned char* p, size_t n) {
    static const char* a="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; std::string o;
    for(size_t i=0;i<n;i+=3){ unsigned v=p[i]<<16; if(i+1<n)v|=p[i+1]<<8; if(i+2<n)v|=p[i+2]; o+=a[(v>>18)&63]; o+=a[(v>>12)&63]; o+=(i+1<n)?a[(v>>6)&63]:'='; o+=(i+2<n)?a[v&63]:'='; } return o;
}
bool hash(const std::string& in, std::vector<unsigned char>& out) {
    BCRYPT_ALG_HANDLE a=nullptr; BCRYPT_HASH_HANDLE h=nullptr; DWORD ob=0,hb=0,cb=0;
    if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)!=0) return false;
    BCryptGetProperty(a,BCRYPT_OBJECT_LENGTH,(PUCHAR)&ob,sizeof(ob),&cb,0); BCryptGetProperty(a,BCRYPT_HASH_LENGTH,(PUCHAR)&hb,sizeof(hb),&cb,0);
    std::vector<unsigned char> obj(ob); out.resize(hb);
    bool ok=BCryptCreateHash(a,&h,obj.data(),ob,nullptr,0,0)==0 && BCryptHashData(h,(PUCHAR)in.data(),(ULONG)in.size(),0)==0 && BCryptFinishHash(h,out.data(),hb,0)==0;
    if(h)BCryptDestroyHash(h); BCryptCloseAlgorithmProvider(a,0); return ok;
}
}

int wmain() {
    const auto root=makeRoot(); const auto trustRoot=root/L"trust"; std::error_code ec; std::filesystem::create_directories(trustRoot,ec); if(ec) return 2;
    bool all=true;
    const std::string integrity="# ZERO package integrity v1\n# files=1 bytes=4\naaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  4  game.bin\n";
    check(writeText(root/L"zero.integrity.sha256",integrity),"integrity_fixture_written",all);

    zero::DisconnectedPublisherTrustProvider disconnected;
    auto r=zero::PackageTrustService::Evaluate(root,zero::PackageTrustPolicy::AllowLocalUnsigned,disconnected);
    check(r.state==zero::PackageTrustState::Unsigned && r.launchAllowed,"unsigned_local_package_allowed",all);

    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_KEY_HANDLE key=nullptr;
    bool crypto=BCryptOpenAlgorithmProvider(&alg,BCRYPT_ECDSA_P256_ALGORITHM,nullptr,0)==0 && BCryptGenerateKeyPair(alg,&key,256,0)==0 && BCryptFinalizeKeyPair(key,0)==0;
    check(crypto,"cng_keypair_generated",all); if(!crypto) return 2;
    ULONG blobSize=0; BCryptExportKey(key,nullptr,BCRYPT_ECCPUBLIC_BLOB,nullptr,0,&blobSize,0); std::vector<unsigned char> blob(blobSize);
    crypto=BCryptExportKey(key,nullptr,BCRYPT_ECCPUBLIC_BLOB,blob.data(),blobSize,&blobSize,0)==0; check(crypto,"public_key_exported",all);
    auto* hdr=reinterpret_cast<BCRYPT_ECCKEY_BLOB*>(blob.data()); const unsigned char* xy=blob.data()+sizeof(BCRYPT_ECCKEY_BLOB);
    const std::string keyJson="{\"publisher_id\":\"zero.test.publisher\",\"key_id\":\"test-key-1\",\"algorithm\":\"ecdsa-p256-sha256\",\"x\":\""+hex(xy,hdr->cbKey)+"\",\"y\":\""+hex(xy+hdr->cbKey,hdr->cbKey)+"\"}";
    check(writeText(trustRoot/L"zero.test.publisher__test-key-1.json",keyJson),"trusted_public_key_written",all);

    std::vector<unsigned char> digest; hash(integrity,digest); ULONG sigSize=0; BCryptSignHash(key,nullptr,digest.data(),(ULONG)digest.size(),nullptr,0,&sigSize,0); std::vector<unsigned char> sig(sigSize);
    crypto=BCryptSignHash(key,nullptr,digest.data(),(ULONG)digest.size(),sig.data(),sigSize,&sigSize,0)==0; check(crypto && sigSize==64,"cng_signature_created",all);
    const std::string env="{\"schema\":1,\"publisher_id\":\"zero.test.publisher\",\"key_id\":\"test-key-1\",\"algorithm\":\"ecdsa-p256-sha256\",\"payload\":\"zero.integrity.sha256\",\"signature\":\""+b64(sig.data(),sigSize)+"\"}";
    check(writeText(root/L"zero.signature.json",env),"signature_envelope_written",all);

    zero::CngPublisherTrustProvider provider(trustRoot);
    r=zero::PackageTrustService::Evaluate(root,zero::PackageTrustPolicy::RequireTrustedPublisher,provider);
    check(r.state==zero::PackageTrustState::TrustedPublisher && r.identityVerified && r.launchAllowed,"real_cng_signature_trusted",all);

    auto tampered=env; auto p=tampered.find("signature\\\":\\\""); (void)p;
    sig[0]^=0x01;
    const std::string badEnv="{\"schema\":1,\"publisher_id\":\"zero.test.publisher\",\"key_id\":\"test-key-1\",\"algorithm\":\"ecdsa-p256-sha256\",\"payload\":\"zero.integrity.sha256\",\"signature\":\""+b64(sig.data(),sigSize)+"\"}";
    writeText(root/L"zero.signature.json",badEnv);
    r=zero::PackageTrustService::Evaluate(root,zero::PackageTrustPolicy::AllowLocalUnsigned,provider);
    check(r.state==zero::PackageTrustState::InvalidSignature && !r.launchAllowed,"invalid_crypto_signature_blocks_launch",all);

    const std::string unknownEnv="{\"schema\":1,\"publisher_id\":\"zero.unknown.publisher\",\"key_id\":\"key-1\",\"algorithm\":\"ecdsa-p256-sha256\",\"payload\":\"zero.integrity.sha256\",\"signature\":\""+b64(sig.data(),sigSize)+"\"}";
    writeText(root/L"zero.signature.json",unknownEnv);
    r=zero::PackageTrustService::Evaluate(root,zero::PackageTrustPolicy::RequireTrustedPublisher,provider);
    check(r.state==zero::PackageTrustState::UntrustedPublisher && !r.launchAllowed,"unknown_publisher_blocked_by_strict_policy",all);

    if(key)BCryptDestroyKey(key); if(alg)BCryptCloseAlgorithmProvider(alg,0); std::filesystem::remove_all(root,ec);
    std::cout << "Result: " << (all?"PASS":"FAIL") << "\n"; return all?0:2;
}
