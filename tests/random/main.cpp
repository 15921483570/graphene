/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Any modified source or binaries are used only with the BitShares network.
 *
 * 2. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 3. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <graphene/app/application.hpp>
#include <graphene/app/plugin.hpp>

#include <graphene/chain/balance_object.hpp>

#include <graphene/time/time.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <graphene/account_history/account_history_plugin.hpp>

#include <fc/thread/thread.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/filesystem/path.hpp>

#define BOOST_TEST_MODULE Test Application
#include <boost/test/included/unit_test.hpp>

#include <cryptopp/ida.h>
#include <cryptopp/smartptr.h>
#include <cryptopp/integer.h>
#include <cryptopp/channels.h>
#include <cryptopp/randpool.h>
#include <cryptopp/files.h>
#include <cryptopp/validate.h>
#include <string>
#include <cryptopp/osrng.h>

#include <cstdlib>

using namespace graphene;
using namespace CryptoPP;
using namespace std;

typedef vector<byte> Bytes;

std::vector<Bytes> SecretShareBytes(const Bytes& secret, int threshold, int nShares)
{
    CryptoPP::AutoSeededRandomPool rng;
    
    CryptoPP::ChannelSwitch *channelSwitch;
    CryptoPP::ArraySource source( secret.data(), secret.size(), false,new CryptoPP::SecretSharing( rng, threshold, nShares, channelSwitch = new CryptoPP::ChannelSwitch) );
    
    std::vector<Bytes> shares( nShares );
    CryptoPP::vector_member_ptrs<CryptoPP::ArraySink> arraySinks( nShares );
    std::string channel;
    for (int i = 0; i < nShares; i++)
    {
        shares[i] = Bytes( 64);
        arraySinks[i].reset( new CryptoPP::ArraySink((byte*)shares[i].data(), shares[i].size()) );
        
        channel = CryptoPP::WordToString<word32>(i);
        arraySinks[i]->Put( (byte *)channel.data(), 4 );
        channelSwitch->AddRoute( channel,*arraySinks[i],CryptoPP::BufferedTransformation::NULL_CHANNEL );
    }
    
    source.PumpAll();
    return shares;
}

Bytes SecretRecoverBytes(std::vector<Bytes>& shares, int threshold)
{
    Bytes bytes( shares[0].size() - sizeof( int ) );
    CryptoPP::SecretRecovery recovery( threshold, new CryptoPP::ArraySink(bytes.data(), bytes.size()) );
    
    CryptoPP::SecByteBlock channel(4);
    for (int i = 0; i < threshold; i++)
    {
        CryptoPP::ArraySource arraySource(shares[i].data(), shares[i].size(), false);
        
        arraySource.Pump(4);
        arraySource.Get( channel, 4 );
        arraySource.Attach( new CryptoPP::ChannelSwitch( recovery, std::string( (char *)channel.begin(), 4) ) );
        
        arraySource.PumpAll();
    }
    
    return bytes;
}


void SecretShareFile(int threshold, int nShares, const char *filename, const char *seed)
{

    RandomPool rng;
    rng.IncorporateEntropy((byte *)seed, strlen(seed));
    
    ChannelSwitch *channelSwitch;
    FileSource source(filename, false, new SecretSharing(rng, threshold, nShares, channelSwitch = new ChannelSwitch));
    
    vector_member_ptrs<FileSink> fileSinks(nShares);
    string channel;
    for (int i=0; i<nShares; i++)
    {
        char extension[5] = ".000";extension[1]='0'+byte(i/100);extension[2]='0'+byte((i/10)%10);extension[3]='0'+byte(i%10);
        
        fileSinks[i].reset(new FileSink((string(filename)+extension).c_str()));
        
        channel = WordToString<word32>(i);
        fileSinks[i]->Put((const byte *)channel.data(), 4);
        channelSwitch->AddRoute(channel, *fileSinks[i], DEFAULT_CHANNEL);
        
        //cout << i<<": "<< channel << endl;
        
    }
    
    
    source.PumpAll();
}

void SecretRecoverFile(int threshold, const char *outFilename, char *const *inFilenames)
{
    SecretRecovery recovery(threshold, new FileSink(outFilename));
    
    vector_member_ptrs<FileSource> fileSources(threshold);
    SecByteBlock channel(4);
    int i;
    for (i=0; i<threshold; i++)
    {
        fileSources[i].reset(new FileSource(inFilenames[i], false));
        fileSources[i]->Pump(4);
        fileSources[i]->Get(channel, 4);
        fileSources[i]->Attach(new ChannelSwitch(recovery, string((char *)channel.begin(), 4)));
    }
    
    while (fileSources[0]->Pump(256))
        for (i=1; i<threshold; i++)
            fileSources[i]->Pump(256);
    
    for (i=0; i<threshold; i++)
        fileSources[i]->PumpAll();
}


namespace BlockUtil {
    int generateBlockNum()
    {
        static int num = 0;
        return ++num;
    }
    
    
    class SecretSliceDemo
    {
    public:
        int _sender, _recver;
        Bytes _slice;
    };

    
    class BlockDemo
    {
    public:
        BlockDemo(int n): _num(n){}
        
        int _num;
    private:
        
        /*
         1. Witness's  hash(_secret)
         2. N pieces of the _secret ( encrypted by other Witness's public key)
         3. Decrypted _secret slices (encrpted by my private key)
         */
        fc::sha256 _hash_secret;
        vector<SecretSliceDemo> _encrypt_secret_list;
        vector<SecretSliceDemo> _decrypt_secret_list;
        
    };
    
    vector<BlockDemo*> global_block_list;
    
    BlockDemo* generate_block(fc::ecc::private_key)
    {
        BlockDemo* blk = new BlockDemo(generateBlockNum());
        
        global_block_list.push_back(blk);
        
        return blk;
    }
    

}

#define WitnessDemoNum 10


class WitnessDemo
{
public:
    WitnessDemo(int id, int s): _id(id), _secret(s)
    {
        _priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(_id) );
    };
    
    void creat_one_block()
    {
        // split _secret into N pieces
        // publish hash(_secret)
        // decrypt the secret slices of other Witness
        
        auto b = BlockUtil::generate_block(_priv_key);
        
        cout << _id << ": creat block " << b->_num << endl;
    }
    
private:
    int _id,  _secret;
    fc::ecc::private_key _priv_key;
    
    //vector<Bytes> v;
};

vector<WitnessDemo*> global_witness_list;

void init_witness()
{
    std::srand(std::time(NULL));
    
    for (int i=1; i<=WitnessDemoNum; i++) {
        int secret = rand();
        WitnessDemo* wt = new WitnessDemo(i, secret);
        global_witness_list.push_back(wt);
    }
    
}

void witness_generate_blocks()
{
    for (int i = 1; i<=100; i++) {
        int candidate = rand()%WitnessDemoNum;
        
        global_witness_list[candidate]->creat_one_block();
    }
}



BOOST_AUTO_TEST_CASE( random_demo_test )
{
    init_witness();
    
    witness_generate_blocks();
}

//BOOST_AUTO_TEST_CASE( secret_share_demo )
void secret_share_demo()
{
   using namespace graphene::chain;
   using namespace graphene::app;
    
   try {
       
      BOOST_TEST_MESSAGE( "Checking random test" );
       
       cout<<"input you secret:";
       string mysecret;
       getline(cin, mysecret);
       
       Bytes sv;
       
       std::copy( mysecret.begin(), mysecret.end(), std::back_inserter(sv));
       
       std::vector<Bytes> shares = SecretShareBytes(sv, 2, 5);
       
       for(auto i=0; i<shares.size(); i++)
       {
           for(auto ch: shares[i])
           {
               cout << hex << (int)ch;
           }
           cout <<endl;
       }
       
       std::vector<Bytes> shares_rec;
       shares_rec.push_back(shares[1]);
       shares_rec.push_back(shares[3]);
       Bytes mys_rec = SecretRecoverBytes(shares_rec, 2);
       for(auto ch: mys_rec)
       {
           cout << ch;
       }
       
       
   } catch( fc::exception& e ) {
      edump((e.to_detail_string()));
      throw;
   }
}
