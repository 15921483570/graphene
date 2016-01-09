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
#include <graphene/chain/protocol/memo.hpp>
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
        shares[i] = Bytes( 12);
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
    
    class EncryptSecretSlice
    {
    public:
        EncryptSecretSlice(int s, int r, chain::memo_data d):_sender(s), _recver(r), _md(d){}
    public:
        int _sender, _recver;
        chain::memo_data _md;
    };
    
    class DescryptSecretSlice
    {
    public:
        DescryptSecretSlice(int d, int o, Bytes s):_descrypter(d), _owner(o), _secret(s){}
    public:
        int _descrypter, _owner;
        Bytes _secret;
    };

    class BlockDemo
    {
    public:
        BlockDemo(int n): _num(n){}
        
        int _num;
    
    public:
        
        /*
         1. Witness's  hash(_secret)
         2. N pieces of the _secret ( encrypted by other Witness's public key)
         3. Decrypted _secret slices (encrpted by my private key)
         */
        fc::sha256 _hash_secret;
        vector<EncryptSecretSlice>  _encrypt_secret_list;
        vector<DescryptSecretSlice> _descrypt_secret_list;
        
        Bytes _recover_secret;
        int _secret_owner;
        
    };
    
    vector<BlockDemo*> global_block_list;
    
    BlockDemo* generate_block(
                              fc::ecc::private_key prv_key,
                              fc::sha256 hash_s,
                              vector<EncryptSecretSlice>& enc_ss,
                              vector<DescryptSecretSlice> dec_sec,
                              int sec_owner,
                              Bytes secret)
    {
        BlockDemo* blk = new BlockDemo(generateBlockNum());
        blk->_hash_secret = hash_s;
        blk->_encrypt_secret_list = enc_ss;
        blk->_descrypt_secret_list = dec_sec;
        blk->_secret_owner = sec_owner;
        blk->_recover_secret = secret;
        
        global_block_list.push_back(blk);
        
        return blk;
    }
    

}

#define WitnessDemoNum 10
#define WitnessSecretThreshold 2

class WitnessDemo;
vector<WitnessDemo*> global_witness_list;

class WitnessDemo
{
public:
    WitnessDemo(int id, int s): _id(id), _secret(s)
    {
        _priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(_id) );
    };
    
    void creat_one_block()
    {
        // Missions here:
        // 1. split _secret into N pieces
        // 2. publish hash(_secret)
        // 3. decrypt the secret slices from other Witness
        
        // 1.
        fc::sha256 hash_my_secret = fc::sha256::hash(_secret);
        
        // 2.
        Bytes secret_bytes;
        for (int i = 3; i >= 0; i--)
        {
            byte b = (_secret >> 8 * i) & 0xFF;
            secret_bytes.push_back(b);
        }
        
        vector<Bytes> split_secrets = SecretShareBytes(secret_bytes, WitnessSecretThreshold, WitnessDemoNum);
        
        vector<BlockUtil::EncryptSecretSlice> encryptSS_vec;
        
        for (int i=1; i<=WitnessDemoNum; i++) {
            if(i == _id) continue;
            
            WitnessDemo* wt = global_witness_list[i-1];
            
            chain::memo_data m;
            
            auto receiver_pub_key = wt->get_public_key();
            auto sender_prv_key = _priv_key;
            auto sender_pub_key = _priv_key.get_public_key();
            
            string secret_msg(split_secrets[i-1].begin(), split_secrets[i-1].end());
        
            // encrypt the split secret
            m.from = sender_pub_key;
            m.to = receiver_pub_key;
            m.set_message(sender_prv_key, receiver_pub_key, secret_msg);
            
            BlockUtil::EncryptSecretSlice ess(this->_id, i, m);
            
            encryptSS_vec.push_back(ess);
        }
        
        // 3.
        vector<BlockUtil::DescryptSecretSlice> decrypt_secrets;
        for(auto ess: _ess_list)
        {
            assert(ess._recver == this->_id);
            
            chain::memo_data md = ess._md;
            
            string secret_msg = md.get_message(_priv_key, global_witness_list[ess._sender-1]->get_public_key());
            
            BlockUtil::DescryptSecretSlice ds(_id, ess._sender, Bytes(secret_msg.begin(), secret_msg.end()));
            decrypt_secrets.push_back(ds);
            
            //
            _other_witness_secrets[ess._sender].push_back(Bytes(secret_msg.begin(), secret_msg.end()));
        }
        
        
        int sec_owner_to_store = -1;
        Bytes rec_secret;
        
        // recover the secret, if we got enough info here
        for(auto ent: _other_witness_secrets)
        {
            vector<Bytes>& secret_slice_list = ent.second;
            if(secret_slice_list.size()==WitnessSecretThreshold)
            {
                Bytes sec_rec = SecretRecoverBytes(secret_slice_list, WitnessSecretThreshold);
                
                cout << "\t\t"<< ent.first<<"'s secret is recovered : "; printHexBytes(cout, sec_rec); cout << endl;
                //fc::usleep(fc::milliseconds(1000));
                
                _other_witness_secrets[ent.first].clear();
                
                sec_owner_to_store = ent.first;
                rec_secret = sec_rec;
            }
            
            // secret is already recovered
            if (secret_slice_list.size() > WitnessSecretThreshold)
            {
                //secret_slice_list.clear();
                _other_witness_secrets[ent.first].clear();
            }
        }
        
        for(auto ent: _other_witness_secrets)
        {
            int sec_owner = ent.first;
            vector<Bytes> secret_slice_list = ent.second;
            if(secret_slice_list.size() == 0) continue;
            
            cout << "\t\t\t owner is " << sec_owner <<endl;
            
            for (auto bytes : secret_slice_list)
            {
                printHexBytes(cout, bytes);
            }
            cout << endl;
        }
        
        
        // generate block
        auto b = BlockUtil::generate_block(_priv_key, hash_my_secret, encryptSS_vec,  decrypt_secrets, sec_owner_to_store, rec_secret);
        
        // dump details
        cout << "witness "<< _id << ": creat block " << b->_num << endl;
        cout << "\t\t my secrect is :";
        printHexBytes(cout, secret_bytes);
        
        cout << "\t\t my split secrets are: " <<endl;
        for(auto bytes: split_secrets)
        {
            cout << "\t\t\t"; printHexBytes(cout, bytes);
        }
        
    }
    
    void on_new_block()
    {
        auto blk = BlockUtil::global_block_list.back();
        for (auto es: blk->_encrypt_secret_list)
        {
            if(es._recver == this->_id) //it is for me
            {
                _ess_list.push_back(es);
            }
        }
        
        for (auto ds: blk->_descrypt_secret_list)
        {
            _other_witness_secrets[ds._owner].push_back(ds._secret);
        }
        
        if(blk->_secret_owner != -1)
        {
            _other_witness_secrets[blk->_secret_owner].clear();
        }
    }
    
    void printHexBytes(std::ostream& x, Bytes bytes)
    {
        ios::fmtflags f(x.flags());
        
        for(auto b: bytes) x<<hex<<(short)b;
        cout<<endl;
        x.flags(f);
    }
    
    fc::ecc::public_key get_public_key()
    {
        return _priv_key.get_public_key();
    }
    
    void reset_secret(int s)
    {
        _secret = s;
    }
    
    void reset_other_witness_secrets()
    {
        for(auto ent: _other_witness_secrets)
        {
            //int sec_owner = ent.first;
            
            vector<Bytes> secret_slice_list = ent.second;
            if(secret_slice_list.size() >=  WitnessSecretThreshold)
                _other_witness_secrets[ent.first].clear();;

        }
    }
    
private:
    int _id,  _secret;
    fc::ecc::private_key _priv_key;
    
    vector<BlockUtil::EncryptSecretSlice> _ess_list;
    
    map<int, vector<Bytes>> _other_witness_secrets;
};



void init_witness()
{
    std::srand(std::time(NULL));
    
    for (int i=1; i<=10; i++) {
        int secret = rand();
        WitnessDemo* wt = new WitnessDemo(i, secret);
        global_witness_list.push_back(wt);
    }
}

void reset_witness_secret()
{
    for (auto witness : global_witness_list)
    {
        int secret = rand();
        witness->reset_secret(secret);
    }
}

void reset_witness_cached_secrets()
{
    for (auto witness : global_witness_list)
    {
        witness->reset_other_witness_secrets();
    }
}

void witness_generate_blocks()
{
    vector<int> witness_schedule;
    for (int i=1; i<=WitnessDemoNum; ++i) witness_schedule.push_back(i);
  
    for (int i = 1; i<=2; i++) // rounds
    {
        //std::random_shuffle(witness_schedule.begin(), witness_schedule.end());
        reset_witness_secret();
        reset_witness_cached_secrets();
        
        for(auto j:witness_schedule)
        {
            global_witness_list[j-1]->creat_one_block();
            fc::usleep(fc::milliseconds(500));
            
            
            for (auto k: witness_schedule)
            {
                if(k == j) continue;// skip self
                global_witness_list[k-1]->on_new_block();
            }
        }
   
        
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
