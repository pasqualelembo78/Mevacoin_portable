{
    "poolHost": "http://pool.mevacoin.it",
	
    "coin": "Mevacoin",
    "symbol": "MVC",
    "coinUnits": 100000000,
    "coinDecimalPlaces": 8,
    "coinDifficultyTarget": 30,
    "blockchainExplorer": "https://explorer.mevacoin.it/block.html?hash={id}",
    "transactionExplorer": "https://explorer.mevacoin.it/block.html?hash={id}",

    "daemonType": "mevacoind",
    "cnAlgorithm": "cryptonight",
    "cnVariant": 1,
    "cnBlobType": 0,

    "logging": {
        "files": {
            "level": "info",
            "directory": "logs",
            "flushInterval": 5
        },
        "console": {
            "level": "info",
            "colors": true
        }
    },
    "hashingUtil": true,
    "childPools": null,
    "poolServer": {
        "enabled": true,
	"mergedMining": false,
        "clusterForks": "auto",
        "poolAddress": " bicktHtkW4ASubtkXi3QQCRFApZwEuchaKpJdmBHYehu2LSywGKgChG7r3Z2i3n16KUuJ5rXKtf3aWrn26ogWqzw4ym4zuqkRC",
        "intAddressPrefix": null,
        "blockRefreshInterval": 600,
        "minerTimeout": 600,
        "sslCert": "./cert.pem",
        "sslKey": "./privkey.pem",
        "sslCA": "./chain.pem",
        "ports": [
            {
                "port": 3333,
                "difficulty": 500,
                "desc": "Low end hardware"
            },
            {
                "port": 4444,
                "difficulty": 1500,
                "desc": "Mid range hardware"
            },
            {
                "port": 5555,
                "difficulty": 2500,
                "desc": "High end hardware"
            },
            {
                "port": 7777,
                "difficulty": 50000,
                "desc": "Cloud-mining / NiceHash"
            },
            {
                "port": 8888,
                "difficulty": 2500,
                "desc": "Hidden port",
                "hidden": true
            },
            {
                "port": 9999,
                "difficulty": 2000,
                "desc": "SSL connection",
                "ssl": true
            }
        ],
        "varDiff": {
            "minDiff": 5000,
            "maxDiff": 100000000,
            "targetTime": 30,
            "retargetTime": 30,
            "variancePercent": 30,
            "maxJump": 100
        },
        "paymentId": {
            "addressSeparator": "+"
        },
	"separators": [{"value":"+","desc":"plus"}, {"value":".","desc":"dot"}],
        "fixedDiff": {
            "enabled": true,
            "addressSeparator": "."
        },
        "shareTrust": {
            "enabled": true,
            "min": 10,
            "stepDown": 3,
            "threshold": 10,
            "penalty": 30
        },
        "banning": {
            "enabled": false,
            "time": 120,
            "invalidPercent": 80,
            "checkThreshold": 30
        },
        "slushMining": {
            "enabled": false,
            "weight": 300,
            "blockTime": 60,
            "lastBlockCheckRate": 1
         }
    },

    "payments": {
        "enabled": true,
        "interval": 600,
        "maxAddresses": 50,
        "mixin": 3,
        "priority": 0,
        "transferFee": 50,
        "dynamicTransferFee": true,
        "minerPayFee" : true,
        "minPayment": 10000,
        "maxTransactionAmount": 100000000,
        "denomination": 100
    },

    "blockUnlocker": {
        "enabled": true,
        "interval": 30,
        "depth": 40,
        "poolFee": 1,
        "devDonation": 0.2,
        "networkFee": 0.2,
        "fixBlockHeightRPC": false
    },

    "api": {
        "enabled": true,
        "hashrateWindow": 600,
        "updateInterval": 5,
        "bindIp": "0.0.0.0",
        "port": 8070,
        "blocks": 30,
        "payments": 30,
        "password": "your_password",
        "ssl": false,
        "sslPort": 8119,
        "sslCert": "./cert.pem",
        "sslKey": "./privkey.pem",
        "sslCA": "./chain.pem",
        "trustProxyIP": true
    },

    "daemon": {
        "host": "127.0.0.1",
        "port": 18081
    },

    "wallet": {
        "host": "127.0.0.1",
        "port": 18081
    },

    "redis": {
        "host": "127.0.0.1",
        "port": 6379,
        "auth": null,
        "db": 0,
        "cleanupInterval": 15
    },

    "notifications": {
        "emailTemplate": "email_templates/default.txt",
        "emailSubject": {
            "emailAdded": "Your email was registered",
            "workerConnected": "Worker %WORKER_NAME% connected",
            "workerTimeout": "Worker %WORKER_NAME% stopped hashing",
            "workerBanned": "Worker %WORKER_NAME% banned",
            "blockFound": "Block %HEIGHT% found !",
            "blockUnlocked": "Block %HEIGHT% unlocked !",
            "blockOrphaned": "Block %HEIGHT% orphaned !",
            "payment": "We sent you a payment !"
        },
        "emailMessage": {
            "emailAdded": "Your email has been registered to receive pool notifications.",
            "workerConnected": "Your worker %WORKER_NAME% for address %MINER% is now connected from ip %IP%.",
            "workerTimeout": "Your worker %WORKER_NAME% for address %MINER% has stopped submitting hashes on %LAST_HASH%.",
            "workerBanned": "Your worker %WORKER_NAME% for address %MINER% has been banned.",
            "blockFound": "Block found at height %HEIGHT% by miner %MINER% on %TIME%. Waiting maturity.",
            "blockUnlocked": "Block mined at height %HEIGHT% with %REWARD% and %EFFORT% effort on %TIME%.",
            "blockOrphaned": "Block orphaned at height %HEIGHT% :(",
            "payment": "A payment of %AMOUNT% has been sent to %ADDRESS% wallet."
        },
        "telegramMessage": {
            "workerConnected": "Your worker _%WORKER_NAME%_ for address _%MINER%_ is now connected from ip _%IP%_.",
            "workerTimeout": "Your worker _%WORKER_NAME%_ for address _%MINER%_ has stopped submitting hashes on _%LAST_HASH%_.",
            "workerBanned": "Your worker _%WORKER_NAME%_ for address _%MINER%_ has been banned.",
            "blockFound": "*Block found at height* _%HEIGHT%_ *by miner* _%MINER%_*! Waiting maturity.*",
            "blockUnlocked": "*Block mined at height* _%HEIGHT%_ *with* _%REWARD%_ *and* _%EFFORT%_ *effort on* _%TIME%_*.*",
            "blockOrphaned": "*Block orphaned at height* _%HEIGHT%_ *:(*",
            "payment": "A payment of _%AMOUNT%_ has been sent."
        }
    },

    "email": {
        "enabled": false,
        "fromAddress": "your@email.com",
        "transport": "sendmail",
        "sendmail": {
            "path": "/usr/sbin/sendmail"
        },
        "smtp": {
            "host": "smtp.example.com",
            "port": 587,
            "secure": false,
            "auth": {
                "user": "username",
                "pass": "password"
            },
            "tls": {
                "rejectUnauthorized": false
            }
        },
        "mailgun": {
            "key": "your-private-key",
            "domain": "mg.yourdomain"
        }
    },

    "telegram": {
        "enabled": false,
        "token": "",
        "channel": "",
        "channelStats": {
            "enabled": false,
            "interval": 30
        },
        "botCommands": {
            "stats": "/stats",
            "enable": "/enable",
            "disable": "/disable"
        }
    },
    
    "monitoring": {
        "daemon": {
            "checkInterval": 60,
            "rpcMethod": "getblockcount"
        },
        "wallet": {
            "checkInterval": 60,
            "rpcMethod": "getbalance"
        }
    },

    "prices": {
        "source": "tradeogre",
        "currency": "USD"
    },

    "charts": {
        "pool": {
            "hashrate": {
                "enabled": true,
                "updateInterval": 60,
                "stepInterval": 1800,
                "maximumPeriod": 86400
            },
            "miners": {
                "enabled": true,
                "updateInterval": 60,
                "stepInterval": 1800,
                "maximumPeriod": 86400
            },
            "workers": {
                "enabled": true,
                "updateInterval": 60,
                "stepInterval": 1800,
                "maximumPeriod": 86400
            },
            "difficulty": {
                "enabled": true,
                "updateInterval": 1800,
                "stepInterval": 10800,
                "maximumPeriod": 604800
            },
            "price": {
                "enabled": true,
                "updateInterval": 1800,
                "stepInterval": 10800,
                "maximumPeriod": 604800
            },
            "profit": {
                "enabled": true,
                "updateInterval": 1800,
                "stepInterval": 10800,
                "maximumPeriod": 604800
            }
        },
        "user": {
            "hashrate": {
                "enabled": true,
                "updateInterval": 180,
                "stepInterval": 1800,
                "maximumPeriod": 86400
            },
            "worker_hashrate": {
                "enabled": true,
                "updateInterval": 60,
                "stepInterval": 60,
                "maximumPeriod": 86400
            },
            "payments": {
                "enabled": true
            }
        },
        "blocks": {
            "enabled": true,
            "days": 30
        }
    }
}
