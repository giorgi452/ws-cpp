#!/bin/bash
# test_parameters.sh - Test URL and query parameter parsing

COLOR_GREEN='\033[0;32m'
COLOR_BLUE='\033[0;34m'
COLOR_YELLOW='\033[1;33m'
COLOR_RED='\033[0;31m'
COLOR_RESET='\033[0m'

BASE_URL="http://localhost:8080"

echo -e "${COLOR_BLUE}================================${COLOR_RESET}"
echo -e "${COLOR_BLUE}Parameter Parsing Tests${COLOR_RESET}"
echo -e "${COLOR_BLUE}================================${COLOR_RESET}"
echo ""

# Function to test endpoint
test_endpoint() {
    local name=$1
    local url=$2
    local method=${3:-GET}
    local data=$4

    echo -e "${COLOR_YELLOW}Test: $name${COLOR_RESET}"
    echo "URL: $url"
    echo "Method: $method"

    if [ -n "$data" ]; then
        response=$(curl -s -X $method "$url" -d "$data")
    else
        response=$(curl -s -X $method "$url")
    fi

    echo "Response:"
    echo "$response" | python3 -m json.tool 2>/dev/null || echo "$response"
    echo ""
    echo "---"
    echo ""
}

echo -e "${COLOR_GREEN}Basic Routes${COLOR_RESET}"
test_endpoint "Root" "$BASE_URL/"
test_endpoint "Status" "$BASE_URL/api/status"

echo -e "${COLOR_GREEN}Path Parameters${COLOR_RESET}"
test_endpoint "Get User" "$BASE_URL/users/123"
test_endpoint "Get User Post" "$BASE_URL/users/42/posts/789"
test_endpoint "Delete User" "$BASE_URL/users/999" "DELETE"

echo -e "${COLOR_GREEN}Query Parameters${COLOR_RESET}"
test_endpoint "Simple Search" "$BASE_URL/search?q=test"
test_endpoint "Search with Pagination" "$BASE_URL/search?q=laptop&page=2&limit=5"
test_endpoint "Search with Sort" "$BASE_URL/search?q=phone&sort=price&page=1"

echo -e "${COLOR_GREEN}Combined Parameters${COLOR_RESET}"
test_endpoint "Product without Query" "$BASE_URL/products/electronics/laptop-001"
test_endpoint "Product with Reviews" "$BASE_URL/products/electronics/phone-001?reviews=true"
test_endpoint "Product Full Details" "$BASE_URL/products/books/isbn-123?reviews=true&format=detailed"

echo -e "${COLOR_GREEN}URL Encoding${COLOR_RESET}"
test_endpoint "Encoded Spaces" "$BASE_URL/search?q=hello%20world"
test_endpoint "Encoded Plus" "$BASE_URL/search?q=hello+world"
test_endpoint "Special Characters" "$BASE_URL/search?q=C%2B%2B%20programming"

echo -e "${COLOR_GREEN}POST with Parameters${COLOR_RESET}"
test_endpoint "Update User" "$BASE_URL/users/123/update" "POST" '{"name":"Alice","email":"alice@example.com"}'
test_endpoint "Login" "$BASE_URL/login" "POST" "testuser"

echo -e "${COLOR_GREEN}Debug Endpoint${COLOR_RESET}"
test_endpoint "Debug - All Parameters" "$BASE_URL/api/params?foo=bar&test=123&debug"

echo -e "${COLOR_GREEN}Edge Cases${COLOR_RESET}"
test_endpoint "Empty Query Param" "$BASE_URL/search?q="
test_endpoint "Multiple Same Param" "$BASE_URL/search?tag=cpp&tag=async"
test_endpoint "No Query Value" "$BASE_URL/search?debug"

echo -e "${COLOR_GREEN}Error Cases${COLOR_RESET}"
test_endpoint "404 - Not Found" "$BASE_URL/nonexistent"
test_endpoint "404 - Wrong Path" "$BASE_URL/users/123/invalid"

echo ""
echo -e "${COLOR_BLUE}================================${COLOR_RESET}"
echo -e "${COLOR_BLUE}Tests Complete${COLOR_RESET}"
echo -e "${COLOR_BLUE}================================${COLOR_RESET}"
echo ""
echo "Check the responses above to verify:"
echo "  ✓ Path parameters extracted correctly"
echo "  ✓ Query parameters parsed correctly"
echo "  ✓ URL encoding/decoding works"
echo "  ✓ Combined parameters work together"
echo "  ✓ Error handling returns proper 404s"
echo ""
