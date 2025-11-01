-- utility.lua

-- 코루틴 내에서 지정된 시간(초) 동안 대기
function Wait(seconds)
    if not coroutine.running() then
        error("Wait() can only be called inside a coroutine")
        return
    end

    coroutine.yield(seconds)
end

-- 코루틴 내에서 1프레임 대기
function WaitFrame()
    if not coroutine.running() then
        error("WaitFrame() can only be called inside a coroutine")
        return
    end
    
    coroutine.yield()
end
