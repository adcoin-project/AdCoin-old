# Mining Simulation

Here we use docker to set up a local AdCoin mining pool on the regtest network. This network starts with a zero-difficulty, allowing for instant blocks.

The mining simulator can be used to test and demonstrate new algorithms like the difficulty regulator.

Unlike default regtest, we've implemented some patches in AdCoin that allow a normal growth in difficulty (like it would on main net) Because of this, we are able to start with a very low difficulty and mine quickly. We can then use the mining power we have at our disposal as if it's the only equipment in the network.

This allows us to:

- Test the difficulty regulator.
- Test if the given difficulty, combined with the speed of our equipment results in the desired duration to create a block (i.e in adcoin each block should be made 10 minutes after the last one)
- Test if a given hash function (i.e scrypt) performs well within the difficulty regulator.
- Use any given stratum-supported mining client (cgminer, cpuminer, etc) to test, as we are basically our own mining pool.

When setting up the simulator for the first time, it premines 101 blocks instantly so we have around 70 milion adcoins to spend.

Additionally, the simulator also contains a spendbot that sends random transactions after x seconds to always keep pending transactions in the air.

## Get started

- Compile AdCoin for linux (according to the normal build instructions, you can also use a binary version so you dont have to compile yourself) and move the binaries `adcoin-cli` and `litcoind` to `docker/bin`.
- Install docker and docker-compose if you haven't already: <https://docs.docker.com/engine/installation/> and <https://docs.docker.com/compose/install/>
- Now inside the docker directory, run `docker-compose up -d` and wait for the docker VM to build.
- When the build is done, the image should've automatically started. You can now point any random scrypt miner to the mining simulator. By default there are 2 ports for connecting to the local pool: 3032 and 3256\. For instance, with cpuminer (<https://github.com/pooler/cpuminer>) you'd call: `./minerd -o stratum+tcp://localhost:3256`.

## Tips

- By running `docker-compose logs -f` from the docker-folder in this repo, you can see realtime logs from the simulator.
