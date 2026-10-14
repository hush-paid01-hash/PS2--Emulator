#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace retroshelf {

struct EeGifDmaRegisters {
    std::uint32_t chcr{0};
    std::uint32_t madr{0};
    std::uint32_t qwc{0};
    std::uint32_t tadr{0};
    std::uint32_t asr0{0};
    std::uint32_t asr1{0};
    std::uint8_t asp{0};
};

class EeGifDma {
public:
    enum class State { Idle, Running, Completed, Error };
    enum class Interrupt { Completion, Tag, Error };
    using PayloadHandler = std::function<void(const std::vector<std::uint8_t>&)>;
    using InterruptHandler = std::function<void(Interrupt)>;

    static constexpr std::size_t QwordSize = 16;
    static constexpr std::size_t MaxChainTags = 65536;
    static constexpr std::size_t MaxTransferBytes = 64 * 1024 * 1024;

    EeGifDma(std::vector<std::uint8_t>& ee_ram, PayloadHandler payload_handler,
             InterruptHandler interrupt_handler = {});

    EeGifDmaRegisters registers;
    State state() const;
    const std::string& error() const;
    bool start();

private:
    bool transfer(std::uint32_t address, std::uint32_t qwc);
    bool fail(const std::string& message);
    bool valid_range(std::uint32_t address, std::size_t size) const;
    void interrupt(Interrupt reason);

    std::vector<std::uint8_t>& ee_ram_;
    PayloadHandler payload_handler_;
    InterruptHandler interrupt_handler_;
    State state_{State::Idle};
    std::string error_;
};

} // namespace retroshelf
