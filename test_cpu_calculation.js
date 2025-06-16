// 测试CPU时间计算逻辑
function calculateCPUTime(memorySize) {
  return Math.max(200, Math.min(1000, Math.floor(memorySize / 32)));
}

// 测试不同内存大小的CPU时间计算
const testCases = [
  { memory: 1024, expected: 'should be 200 (minimum)' },      // 1024/32 = 32, but min is 200
  { memory: 6400, expected: 'should be 200 (minimum)' },      // 6400/32 = 200
  { memory: 16000, expected: 'should be 500' },               // 16000/32 = 500
  { memory: 32000, expected: 'should be 1000 (maximum)' },    // 32000/32 = 1000
  { memory: 64000, expected: 'should be 1000 (maximum)' },    // 64000/32 = 2000, but max is 1000
];

console.log('CPU时间计算测试结果:');
console.log('公式: Math.max(200, Math.min(1000, Math.floor(memorySize / 32)))');
console.log('范围: 200ms - 1000ms');
console.log('');

testCases.forEach(testCase => {
  const result = calculateCPUTime(testCase.memory);
  console.log(`内存大小: ${testCase.memory} bytes`);
  console.log(`计算结果: ${result}ms`);
  console.log(`预期: ${testCase.expected}`);
  console.log('---');
});

// 验证边界条件
console.log('边界条件验证:');
console.log(`最小值测试 (内存=100): ${calculateCPUTime(100)}ms (应该是200)`);
console.log(`临界值测试 (内存=6400): ${calculateCPUTime(6400)}ms (应该是200)`);
console.log(`中间值测试 (内存=16000): ${calculateCPUTime(16000)}ms (应该是500)`);
console.log(`临界值测试 (内存=32000): ${calculateCPUTime(32000)}ms (应该是1000)`);
console.log(`最大值测试 (内存=100000): ${calculateCPUTime(100000)}ms (应该是1000)`); 